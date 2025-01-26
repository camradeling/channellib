#include <cstring>
#include "tcpbase.h"
#include "tcpserver.h"
#include "chanpool.h"
//----------------------------------------------------------------------------------------------------------------------
void TcpServerSocket::thread_run()
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
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
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return nullptr;
    return std::move(shared_ptr<TcpPeerSocket>(new TcpPeerSocket(schp)));
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
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return;
    add_wait(outQueue, EPOLLIN);
    add_wait(outCmdQueue, EPOLLIN);
    add_wait(listfd,EPOLLIN);
    struct epoll_event ev;
    int res = 0;
    std::unique_ptr<MessageBuffer> cmdPack = nullptr;
    //ждем ивента с любого из направлений
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
            //брыкнувшись из цикла позакрываем все сокеты ниже
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
                    //брыкнувшись из цикла позакрываем все сокеты ниже
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
                inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peer->clientfd, 0, CHAN_OPEN_PACKET)), true);
                peer->rcvState = IO_START;
                add_wait(peer->clientfd,EPOLLIN);
                peers.insert(make_pair<int,shared_ptr<TcpPeerSocket>>((int)peer->clientfd,std::move(peer)));
                WRITELOG("%s[%d]: peers count %d",alias.c_str(),listfd,peers.size());
            }
        }
        else if(ev.data.fd == outQueue)
        {
            //пришел пакет на отправку
            std::unique_ptr<MessageBuffer> packToSend = outQueue.pop();
            if(!packToSend)//не смогла я
                continue;
            //проверяем, висит ли у нас такой клиент
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
                //пакет с данными
                int res = peerit->second->send_packet(peerit->second->packToSend.get(), IO_START, (packType == CHAN_DATA_PACKET_ZIPPED)?true:false);
                if (res < 0)
                {
                    WRITELOG("%s[%d]: send to socket error (start), errno %d", alias.c_str(),peerit->second->clientfd, errno);
                    delete_wait(peerit->second->clientfd);
                    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
                    peers.erase(peerit);
                    //если мы должны закрыть сокет клиента, но находимся в этот момент в состоянии незаконченной отправки,
                    //то надо восстановить ожидание из outQueue, иначе весь канал накроется пиздой
                    //если мы потом получим из outQueue event для неизвестного сокета, то да и хуй с ним, strange_error и все
                    add_wait(outQueue, EPOLLIN);
                }
                else if (res)
                {
                    //не удалось отправить пакет целиком
                    // будем ждать места в сокете на отправку
                    modify_wait(peerit->second->clientfd, EPOLLIN | EPOLLOUT);
                    //отключим ожидание новых пакетов из очереди
                    delete_wait(outQueue);
                }
                else
                {
                    //пакет успешно отправлен
                    if(peerit->second->packToSend->seqnum)
                        WRITELOGEXTRA("%s[%d]: packet sent, OutSeq:%d",alias.c_str(),peerit->second->clientfd,peerit->second->packToSend->seqnum);
                    peerit->second->packToSend.reset();
                }
            }
            else
            {
                //неизвестный тип пакета игнорируем
                WRITELOG("%s[%d]: unknown packet type %d", alias.c_str(),peerit->second->clientfd);
            }
        }
        else if(ev.data.fd == outCmdQueue)
        {
            cmdPack = outCmdQueue.pop();
            if(!cmdPack)//не смогла я
                continue;
            map<int,shared_ptr<TcpPeerSocket>>::iterator peerit = peers.find(cmdPack->getfd());
            if(peerit == peers.end())
            {
                continue;
            }
            enum MessageType packType = cmdPack->Type();
            if (packType == CHAN_CLOSE_PACKET)
            {
                //закрываем соединение
                WRITELOG("%s[%d]: closing connection by slave request", alias.c_str(),peerit->second->clientfd);
                delete_wait(peerit->second->clientfd);
                inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
                peers.erase(peerit);
                //если мы должны закрыть сокет клиента, но находимся в этот момент в состоянии незаконченной отправки,
                    //то надо восстановить ожидание из outQueue, иначе весь канал накроется пиздой
                    //если мы потом получим из outQueue event для неизвестного сокета, то да и хуй с ним, strange_error и все
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
                //пришли данные из сокета
                int res = peerit->second->recv_packet(&peerit->second->recvPacket, peerit->second->rcvState);
                if (res < 0)
                {
                    //ошибка или закрыто соединение
                    if(res == -2)
                    {//соединение закрыли с той стороны
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
                    //если мы должны закрыть сокет клиента, но находимся в этот момент в состоянии незаконченной отправки,
                    //то надо восстановить ожидание из outQueue, иначе весь канал накроется пиздой
                    //если мы потом получим из outQueue event для неизвестного сокета, то да и хуй с ним, strange_error и все
                    add_wait(outQueue, EPOLLIN);
                }
                else if (res == 0)
                {
                    //пакет полностью принят
                    peerit->second->recvPacket->seqnum = peerit->second->seqnum++;
                    WRITELOGEXTRA("%s[%d]: packet received, InSeq:%d", alias.c_str(),peerit->second->clientfd,peerit->second->recvPacket->seqnum);
                    inQueue.push(std::move(peerit->second->recvPacket),true);
                    peerit->second->rcvState = IO_START;
                }
                else
                {
                    //получили часть пакета, продолжаем прием
                    peerit->second->rcvState = IO_CONTINUE;
                }
            }
            else if (ev.events & EPOLLOUT)
            {
                //можно отправить остатки пакета в сокет
                int res = peerit->second->send_packet(peerit->second->packToSend.get(), IO_CONTINUE);
                if (res < 0)
                {
                    WRITELOG("%s[%d]: send to socket error (continue) %d, errno %d", alias.c_str(),res ,peerit->second->clientfd, errno);
                    delete_wait(peerit->second->clientfd);
                    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(peerit->second->clientfd, 0, CHAN_CLOSE_PACKET)), true);
                    peers.erase(peerit);
                    //если мы должны закрыть сокет клиента, но находимся в этот момент в состоянии незаконченной отправки,
                    //то надо восстановить ожидание из outQueue, иначе весь канал накроется пиздой
                    //если мы потом получим из outQueue event для неизвестного сокета, то да и хуй с ним, strange_error и все
                    add_wait(outQueue, EPOLLIN);
                }
                else if (res == 0)
                {
                    //пакет отправлен, можно получать следующие пакеты из очереди
                    add_wait(outQueue, EPOLLIN);
                    //и отключить ожидание свободного места в сокете
                    modify_wait(peerit->second->clientfd, EPOLLIN);
                    if(peerit->second->packToSend->seqnum)
                        WRITELOGEXTRA("%s[%d]: packet sent, OutSeq:%d",alias.c_str(),peerit->second->clientfd,peerit->second->packToSend->seqnum);
                    peerit->second->packToSend.reset();
                }
                else
                {
                    //пакет опять не влез в сокет, продолжаем ждать место
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
