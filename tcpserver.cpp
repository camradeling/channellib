#include "tcpbase.h"
#include "tcpserver.h"
#include "chanpool.h"
//----------------------------------------------------------------------------------------------------------------------
void TcpServerSocket::thread_run()
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return;
    sigset_t signal_mask;
	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGSEGV);
	pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
	while(!exit)
	{
        listaddr.sin_family = AF_INET;
        listaddr.sin_addr.s_addr = INADDR_ANY;
        listfd = socket(AF_INET, SOCK_STREAM, 0);
        int val = 1;
        if(listfd < 0)
        {
            int err = errno;
            WRITELOG("%s[%d]: listen socket opening failed with error %i",alias.c_str(),listfd, err);
            continue;
        }
        WRITELOG("%s[%d]: opened listen socket",alias.c_str(), listfd);
        if (setsockopt(listfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0)
        {
            WRITELOG("%s[%d]: setsockopt(SO_REUSEADDR) failed",alias.c_str(),listfd);
            close(listfd);
            usleep(1000000);
            continue;
        }
        if(bind(listfd, (struct sockaddr*)&listaddr, sizeof(listaddr)) < 0)
        {
            int err = errno;
            WRITELOG("%s[%d]: bind failed with error %i",alias.c_str(),listfd, err);
            close(listfd);
            continue;
        }
		int listenret = 0;
        if((listenret = listen(listfd, MaxPeers)) < 0)    
        {
			int err = errno;
			WRITELOG("%s[%d]: listen failed with error %i",alias.c_str(),listfd, err);
            close(listfd);
			continue;
		}
        WRITELOG("%s thread: listening on port %d", alias.c_str(),ntohs(listaddr.sin_port));
        do_message_loop();
	}//while not exit
	WRITELOG("%s: exiting",alias.c_str());
}
//----------------------------------------------------------------------------------------------------------------------
TcpServerSocket::~TcpServerSocket()
{
    exit=1;
    clear_and_close();
}
//----------------------------------------------------------------------------------------------------------------------
int TcpServerSocket::clear_and_close()
{
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
shared_ptr<TcpPeerSocket> TcpServerSocket::new_peer()
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return nullptr;
    return std::move(shared_ptr<TcpPeerSocket>(new TcpPeerSocket(schanpool)));
}
//----------------------------------------------------------------------------------------------------------------------
string sockaddr_to_string(struct sockaddr_in sa)
{
    char str[INET_ADDRSTRLEN];
    strcpy(str, inet_ntoa(sa.sin_addr));
    return string(str,strlen(str));
}
//----------------------------------------------------------------------------------------------------------------------
void TcpServerSocket::do_message_loop()
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return;
    add_wait(outQueue, EPOLLIN);
    add_wait(outCmdQueue, EPOLLIN);
    add_wait(listfd,EPOLLIN);
    struct epoll_event ev;
    int res = 0;
    std::unique_ptr<MessageBuffer> cmdPack = nullptr;
    //???????? ???????????? ?? ???????????? ???? ??????????????????????
    while (!exit)
    {
        if ((res = epoll_wait(epollFD, &ev, 1, 10)) < 0)
        {
            if(errno == EINTR)
                continue;
            else
            {
                WRITELOG("%s: epoll_wait error, errno %d", alias.c_str(),errno);
                break;
            }
            //?????????????????????? ???? ?????????? ?????????????????????? ?????? ???????????? ????????
        }
        else if(res == 0)
        {
            continue;
        }
        if(ev.data.fd == listfd)
        {
            shared_ptr<TcpPeerSocket> peer = new_peer();
            peer->clientfd = accept(listfd, (struct sockaddr *)&peer->peer_addr, &peer->peer_addrlen);
            if (peer->clientfd < 0)
            {
                int err = errno;
                WRITELOG("%s[%d]: connection accept failed, clientfd %d, errno %d",alias.c_str(),listfd,peer->clientfd,errno);
                if(errno == 11)
                {
                    WRITELOG("%s[%d]: lets try again",alias.c_str(),listfd);
                    continue;
                }
                else
                {
                    //?????????????????????? ???? ?????????? ?????????????????????? ?????? ???????????? ????????
                    break;
                }
            }
            else if(peers.size() >= MaxPeers)
            {
                WRITELOG("%s[%d]: dropping new incoming connection, too many peers",alias.c_str(),listfd);
                continue;
            }
            else
            {
                struct sockaddr_in addr;
                socklen_t addr_size = sizeof(struct sockaddr_in);
                int res = getpeername(peer->clientfd, (struct sockaddr *)&addr, &addr_size);
                char clientip[20] = {0};
                strncpy(clientip,inet_ntoa(addr.sin_addr),20);
                peer->peeraddr = string(clientip);
                int flags = fcntl(peer->clientfd, F_GETFL, 0);
                if (flags < 0)
                {
                    WRITELOG("%s[%d]: coldn't get socket flags",alias.c_str(),peer->clientfd);
                    continue;
                }
                else if (fcntl(peer->clientfd, F_SETFL, flags | O_NONBLOCK) < 0)
                {
                    WRITELOG("%s[%d]: coldn't set socket O_NONBLOCK flag",alias.c_str(),peer->clientfd);
                    continue;
                }
                enable_keepalive(peer->clientfd);
                WRITELOG("%s[%d]: tcp connection established from %s:%d",alias.c_str(),peer->clientfd,peer->peeraddr.c_str(),ntohs(peer->peer_addr.sin_port));
                inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peer->clientfd, 0, CHAN_OPEN_PACKET,peer->peeraddr)), true);
                peer->rcvState = IO_START;
                add_wait(peer->clientfd,EPOLLIN);
                peers.insert(make_pair<int,shared_ptr<TcpPeerSocket>>((int)peer->clientfd,std::move(peer)));
                WRITELOG("%s[%d]: peers count %d",alias.c_str(),listfd,peers.size());
            }
        }
        else if(ev.data.fd == outQueue)
        {
            //???????????? ?????????? ???? ????????????????
            std::unique_ptr<MessageBuffer> packToSend = outQueue.pop();
            if(!packToSend)//???? ???????????? ??
                continue;
            //??????????????????, ?????????? ???? ?? ?????? ?????????? ????????????
            map<int,shared_ptr<TcpPeerSocket>>::iterator peerit = peers.find(packToSend->getfd());
            if(peerit == peers.end())
            {
                WRITELOG("%s[%d]: no such peer", alias.c_str(),packToSend->getfd());
                continue;
            }
            peerit->second->packToSend = std::move(packToSend);
            enum MessageType packType = peerit->second->packToSend->Type();
            if (packType == CHAN_DATA_PACKET || packType == CHAN_DATA_PACKET_ZIPPED)
            {
                //?????????? ?? ??????????????
                int res = peerit->second->send_packet(peerit->second->packToSend.get(), IO_START, (packType == CHAN_DATA_PACKET_ZIPPED)?true:false);
                if (res < 0)
                {
                    WRITELOG("%s[%d]: send to socket error (start), errno %d", alias.c_str(),peerit->second->clientfd, errno);
                    delete_wait(peerit->second->clientfd);
                    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
                    peers.erase(peerit);
                    //???????? ???? ???????????? ?????????????? ?????????? ??????????????, ???? ?????????????????? ?? ???????? ???????????? ?? ?????????????????? ?????????????????????????? ????????????????,
                    //???? ???????? ???????????????????????? ???????????????? ???? outQueue, ?????????? ???????? ?????????? ?????????????????? ????????????
                    //???????? ???? ?????????? ?????????????? ???? outQueue event ?????? ???????????????????????? ????????????, ???? ???? ?? ?????? ?? ??????, strange_error ?? ??????
                    add_wait(outQueue, EPOLLIN);
                }
                else if (res)
                {
                    //???? ?????????????? ?????????????????? ?????????? ??????????????
                    // ?????????? ?????????? ?????????? ?? ???????????? ???? ????????????????
                    modify_wait(peerit->second->clientfd, EPOLLIN | EPOLLOUT);
                    //???????????????? ???????????????? ?????????? ?????????????? ???? ??????????????
                    delete_wait(outQueue);
                }
                else
                {
                    //?????????? ?????????????? ??????????????????
                    if(peerit->second->packToSend->seqnum)
                        WRITELOGEXTRA("%s[%d]: packet sent, OutSeq:%d",alias.c_str(),peerit->second->clientfd,peerit->second->packToSend->seqnum);
                    peerit->second->packToSend.reset();
                }
            }
            else
            {
                //?????????????????????? ?????? ???????????? ????????????????????
                WRITELOG("%s[%d]: unknown packet type %d", alias.c_str(),peerit->second->clientfd);
            }
        }
        else if(ev.data.fd == outCmdQueue)
        {
            cmdPack = outCmdQueue.pop();
            if(!cmdPack)//???? ???????????? ??
                continue;
            map<int,shared_ptr<TcpPeerSocket>>::iterator peerit = peers.find(cmdPack->getfd());
            if(peerit == peers.end())
            {
                continue;
            }
            enum MessageType packType = cmdPack->Type();
            if (packType == CHAN_CLOSE_PACKET)
            {
                //?????????????????? ????????????????????
                WRITELOG("%s[%d]: closing connection by slave request", alias.c_str(),peerit->second->clientfd);
                delete_wait(peerit->second->clientfd);
                inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
                peers.erase(peerit);
                //???????? ???? ???????????? ?????????????? ?????????? ??????????????, ???? ?????????????????? ?? ???????? ???????????? ?? ?????????????????? ?????????????????????????? ????????????????,
                    //???? ???????? ???????????????????????? ???????????????? ???? outQueue, ?????????? ???????? ?????????? ?????????????????? ????????????
                    //???????? ???? ?????????? ?????????????? ???? outQueue event ?????? ???????????????????????? ????????????, ???? ???? ?? ?????? ?? ??????, strange_error ?? ??????
                add_wait(outQueue, EPOLLIN);
            }
        }
        else
        {
            map<int,shared_ptr<TcpPeerSocket>>::iterator peerit = peers.find(ev.data.fd);
            if(peerit == peers.end())
                continue;
            if (ev.events & EPOLLIN)
            {
                //???????????? ???????????? ???? ????????????
                int res = peerit->second->recv_packet(&peerit->second->recvPacket, peerit->second->rcvState);
                if (res < 0)
                {
                    //???????????? ?????? ?????????????? ????????????????????
                    if(res == -2)
                    {//???????????????????? ?????????????? ?? ?????? ??????????????
                        WRITELOG("%s[%d]: connection closed by remote peer", alias.c_str(),peerit->second->clientfd);
                    }
                    else
                    {
                        WRITELOG("%s[%d]: receive from socket error",alias.c_str(),peerit->second->clientfd);
                        send(peerit->second->clientfd, 0, 0, 0);
                    }
                    delete_wait(peerit->second->clientfd);
                    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
                    peers.erase(peerit);
                    //???????? ???? ???????????? ?????????????? ?????????? ??????????????, ???? ?????????????????? ?? ???????? ???????????? ?? ?????????????????? ?????????????????????????? ????????????????,
                    //???? ???????? ???????????????????????? ???????????????? ???? outQueue, ?????????? ???????? ?????????? ?????????????????? ????????????
                    //???????? ???? ?????????? ?????????????? ???? outQueue event ?????? ???????????????????????? ????????????, ???? ???? ?? ?????? ?? ??????, strange_error ?? ??????
                    add_wait(outQueue, EPOLLIN);
                }
                else if (res == 0)
                {
                    //?????????? ?????????????????? ????????????
                    peerit->second->recvPacket->seqnum = peerit->second->seqnum++;
                    WRITELOGEXTRA("%s[%d]: packet received, InSeq:%d", alias.c_str(),peerit->second->clientfd,peerit->second->recvPacket->seqnum);
                    inQueue.push(std::move(peerit->second->recvPacket),true);
                    peerit->second->rcvState = IO_START;
                }
                else
                {
                    //???????????????? ?????????? ????????????, ???????????????????? ??????????
                    peerit->second->rcvState = IO_CONTINUE;
                }
            }
            else if (ev.events & EPOLLOUT)
            {
                //?????????? ?????????????????? ?????????????? ???????????? ?? ??????????
                int res = peerit->second->send_packet(peerit->second->packToSend.get(), IO_CONTINUE);
                if (res < 0)
                {
                    WRITELOG("%s[%d]: send to socket error (continue) %d, errno %d", alias.c_str(),res ,peerit->second->clientfd, errno);
                    delete_wait(peerit->second->clientfd);
                    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
                    peers.erase(peerit);
                    //???????? ???? ???????????? ?????????????? ?????????? ??????????????, ???? ?????????????????? ?? ???????? ???????????? ?? ?????????????????? ?????????????????????????? ????????????????,
                    //???? ???????? ???????????????????????? ???????????????? ???? outQueue, ?????????? ???????? ?????????? ?????????????????? ????????????
                    //???????? ???? ?????????? ?????????????? ???? outQueue event ?????? ???????????????????????? ????????????, ???? ???? ?? ?????? ?? ??????, strange_error ?? ??????
                    add_wait(outQueue, EPOLLIN);
                }
                else if (res == 0)
                {
                    //?????????? ??????????????????, ?????????? ???????????????? ?????????????????? ???????????? ???? ??????????????
                    add_wait(outQueue, EPOLLIN);
                    //?? ?????????????????? ???????????????? ???????????????????? ?????????? ?? ????????????
                    modify_wait(peerit->second->clientfd, EPOLLIN);
                    if(peerit->second->packToSend->seqnum)
                        WRITELOGEXTRA("%s[%d]: packet sent, OutSeq:%d",alias.c_str(),peerit->second->clientfd,peerit->second->packToSend->seqnum);
                    peerit->second->packToSend.reset();
                }
                else
                {
                    //?????????? ?????????? ???? ???????? ?? ??????????, ???????????????????? ?????????? ??????????
                    WRITELOG("%s[%d]: retry sending packet to peer", alias.c_str(),peerit->second->clientfd);
                }
            }
        }
    }
    delete_wait(listfd);
    delete_wait(outQueue);
    delete_wait(outCmdQueue);
    outQueue.stop_and_clear();
    outCmdQueue.stop_and_clear();
    WRITELOG("%s[%d]: closing listen socket",alias.c_str(), listfd);
    close(listfd);
    map<int,shared_ptr<TcpPeerSocket>>::iterator peerit;
    for(peerit=peers.begin(); peerit != peers.end(); ++peerit)
    {
        WRITELOG("%s[%d]: closing peer socket %d",alias.c_str(),listfd, peerit->second->clientfd);
        delete_wait(peerit->second->clientfd);
        inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
    }
    peers.clear();
}
//----------------------------------------------------------------------------------------------------------------------
TcpPeerSocket::~TcpPeerSocket()
{
    if(clientfd >= 0) 
        close(clientfd);
}
//----------------------------------------------------------------------------------------------------------------------
