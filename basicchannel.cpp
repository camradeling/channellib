#include <pthread.h>
#include <mutex>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "basicchannel.h"
#include "chanpool.h"
//----------------------------------------------------------------------------------------------------------------------
int BasicChannel::init()
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return -1;
    epollFD = epoll_create1(EPOLL_CLOEXEC);
    if (epollFD < 0)
    {
        WRITELOG("%s: epoll_create1 failed", alias.c_str());
        return 0;
    }
    start_thread();
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int BasicChannel::start_thread()
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return -1;
    int32_t  ret;
    pthread_attr_t      threadAttr;
    struct sched_param  threadSchedParam;
    pthread_attr_init (&threadAttr);
    pthread_attr_setschedpolicy(&threadAttr, SCHED_OTHER);
    pthread_attr_setschedparam (&threadAttr, &threadSchedParam);
    ret = pthread_create (&threadID, &threadAttr, BasicChannel::_entry_func, this);
    if (ret < 0)
    {
        WRITELOG("%s: Basic channel thread create error", alias.c_str());
        return -1;
    }
    pthread_setschedparam(threadID, SCHED_RR, &threadSchedParam);
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init (&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init (&chanMut, &mattr);
    if(alias != "")
    {
        ret = pthread_setname_np(threadID, alias.substr(0,14).c_str());
    }
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
BasicChannel::~BasicChannel()
{
    clear_and_close();
    delete_wait(outQueue);
    delete_wait(outCmdQueue);
    delete_wait(clientfd);
    inQueue.stop_and_clear();
    inCmdQueue.stop_and_clear();
    if(epollFD >= 0)
        close(epollFD);
    busy=0;
    exit=1;
    if(threadID != 0)
        pthread_join(threadID,nullptr);
}
//----------------------------------------------------------------------------------------------------------------------
int BasicChannel::send_packet(MessageBuffer *packet, enum io_state state, bool zipped)
{
    uint32_t  packLen = packet->Length();
    uint32_t bytesToSend = packLen;
    if(state == IO_START)
        bytesSent = 0;
    do
    {
        char *sendBuf;
        int sendBufLen;
        sendBuf = packet->Data() + bytesSent;
        sendBufLen = packLen - bytesSent;
        int bytesSentLast = send(clientfd, sendBuf, sendBufLen, 0);
        if (bytesSentLast < 0)
        {
            if(errno == EAGAIN)
                break;
            return -1;
        }
        bytesSent += bytesSentLast;
    } while(bytesSent < bytesToSend);
    return bytesToSend - bytesSent;
}
//----------------------------------------------------------------------------------------------------------------------
int BasicChannel::recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state)
{
    char buffer[1024];
    int bytesRecvdLast = 0;
    string total="";
    do
    {
        bytesRecvdLast = recv(clientfd, buffer, 1024, 0);
        if(bytesRecvdLast < 0)
        {
            if (errno == EAGAIN)
            {
                break;
            }
            return -1;
        }
        else if(bytesRecvdLast == 0)
        {
            return -2;
        }
        total += string(buffer,bytesRecvdLast);
    }
    while(bytesRecvdLast);
    bytesRecvd=bytesRecvdLast;
    packet->reset(new MessageBuffer(clientfd, total.length(), CHAN_DATA_PACKET));
    memcpy(packet->get()->Data(), total.data(),total.length());
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int BasicChannel::modify_wait(int fd, uint32_t events)
{
    struct epoll_event epollEv;
    epollEv.data.fd = fd;
    epollEv.events = events;
    return epoll_ctl(epollFD, EPOLL_CTL_MOD, fd, &epollEv);
}
//----------------------------------------------------------------------------------------------------------------------
int BasicChannel::add_wait(int fd, uint32_t events)
{
    struct epoll_event epollEv;
    epollEv.data.fd = fd;
    epollEv.events = events;
    return epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &epollEv);
}
//----------------------------------------------------------------------------------------------------------------------
int BasicChannel::delete_wait(int fd)
{
    return epoll_ctl(epollFD, EPOLL_CTL_DEL, fd, NULL);
}
//----------------------------------------------------------------------------------------------------------------------
void BasicChannel::do_message_loop()
{
    add_wait(outQueue, EPOLLIN);
    add_wait(outCmdQueue, EPOLLIN);
    add_wait(clientfd, EPOLLIN);
    //отправляем сообщение потоку обработчику, что соединение установлено, mtype = 2
    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(clientfd, 0, CHAN_OPEN_PACKET)), true);
    std::unique_ptr<MessageBuffer> packToSend = nullptr, recvPacket = nullptr, cmdPack = nullptr;
    io_state rcvState = IO_START;
    while (busy)
    {
        shared_ptr<ChanPool> schanpool = chanpool.lock();
        if(!schanpool)
            return;
        struct epoll_event ev;
        int res = 0;
        //ждем ивента с любого из направлений
        if ((res = epoll_wait(epollFD, &ev, 1, 10)) < 0)
        {
            if (errno == EINTR)
                continue;
            WRITELOG("%s: epoll_wait error", alias.c_str());
            break;
        }
        if(!res)
            continue;
        //если получили и без ошибок
        if (ev.data.fd == outQueue)
        {
            //пришел пакет на отправку
            packToSend = outQueue.pop();
            if(!packToSend)//не смогла я
                continue;
            enum MessageType packType = packToSend->Type();
            if (packType == CHAN_DATA_PACKET)
            {
                //пакет с данными
                int res = send_packet(packToSend.get(), IO_START);
                if (res < 0)
                {
                    WRITELOG("%s[%d]: send to socket error (start)", alias.c_str(),clientfd);
                    break;
                }
                else if (res)
                {
                    //не удалось отправить пакет целиком
                    // будем ждать места в сокете на отправку
                    modify_wait(clientfd, EPOLLIN | EPOLLOUT);
                    //отключим ожидание новых пакетов из очереди
                    delete_wait(outQueue);
                }
                else
                {
                    //пакет успешно отправлен
                }
            }
            else if (packType == CHAN_DATA_PACKET_ZIPPED)
            {
                //пакет с данными
                int res = send_packet(packToSend.get(), IO_START,true);
                if (res < 0)
                {
                    WRITELOG("%s[%d]: send to socket error (start)", alias.c_str(),clientfd);
                    break;
                }
                else if (res)
                {
                    //не удалось отправить пакет целиком
                    // будем ждать места в сокете на отправку
                    modify_wait(clientfd, EPOLLIN | EPOLLOUT);
                    //отключим ожидание новых пакетов из очереди
                    delete_wait(outQueue);
                }
                else
                {
                    //пакет успешно отправлен
                }
            }
            else
            {
                //неизвестный тип пакета игнорируем
                WRITELOG("%s[%d]: unknown packet type", alias.c_str(),clientfd);
            }
        }
        else if(ev.data.fd == outCmdQueue)
        {
            cmdPack = outCmdQueue.pop();
            if(!cmdPack)//не смогла я
                continue;
            enum MessageType packType = cmdPack->Type();
            if (packType == CHAN_CLOSE_PACKET)
            {
                //закрываем соединение
                busy = 0;
                WRITELOG("%s[%d]: closing connection by slave request", alias.c_str(),clientfd);
                break;
            }
        }
        else if (ev.data.fd == clientfd)
        {
            if (ev.events & EPOLLIN)
            {
                //пришли данные из сокета
                int res = recv_packet(&recvPacket, rcvState);
                if (res < 0)
                {
                    //ошибка или закрыто соединение
                    if(res == -2)
                    {
                        WRITELOG("%s[%d]: connection closed by remote peer", alias.c_str(),clientfd);
                    }
                    else
                    {
                        WRITELOG("%s[%d]: receive from socket error", alias.c_str(),clientfd);
                    }
                    break;
                }
                else if (res == 0)
                {
                    //пакет полностью принят
                    inQueue.push(std::move(recvPacket),true);
                    rcvState = IO_START;
                }
                else
                {
                    //олучили часть пакета, продолжаем прием
                    rcvState = IO_CONTINUE;
                }
            }
            if (ev.events & EPOLLOUT)
            {
                //можно отправить остатки пакета в сокет
                int res = send_packet(packToSend.get(), IO_CONTINUE);
                if (res < 0)
                {
                    WRITELOG("%s[%d]: send to socket error (continue)", alias.c_str(),clientfd);
                    break;
                }
                else if (res == 0)
                {
                    //пакет отправлен, можно получать следующие пакеты из очереди
                    add_wait(outQueue, EPOLLIN);
                    //и отключить ожидание свободного места в сокете
                    modify_wait(clientfd, EPOLLIN);
                }
                else
                {
                    //пакет опять не влез в сокет, продолжаем ждать место
                }
            }
        }
    }
    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(clientfd, 0, CHAN_CLOSE_PACKET)));
    delete_wait(clientfd);
    delete_wait(outQueue);
    delete_wait(outCmdQueue);
    outQueue.stop_and_clear();
    outCmdQueue.stop_and_clear();
    zipflag=0;
}
//----------------------------------------------------------------------------------------------------------------------
