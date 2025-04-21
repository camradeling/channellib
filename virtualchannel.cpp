#include "virtualchannel.h"
#include "chanpool.h"
#include <random>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
//----------------------------------------------------------------------------------------------------------------------
int VirtualChannel::send_packet(MessageBuffer *packet, enum io_state state, bool zipped)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    WRITELOG("Virtual channel send_packet");
    if (callback)
        callback(packet);
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int VirtualChannel::recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state)
{
    // dummy
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
std::string generate_random_chars(int length) {
    std::string chars = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += chars[dist(gen)];
    }
    return result;
}
//----------------------------------------------------------------------------------------------------------------------
void VirtualChannel::thread_run()
{
    string socket_path = "/tmp/fake_socket.sock" + generate_random_chars(10);

    // Удаляем старый сокет, если он существует
    unlink(socket_path.c_str());

    // Создаем Unix domain socket
    clientfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientfd == -1) {
        std::cerr << "socket() failed: " << std::endl;
        return;
    }

    // Настраиваем адрес сокета
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path)-1);

    // Привязываем сокет к адресу
    if (bind(clientfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "bind() failed: " << std::endl;
        close(clientfd);
        return;
    }

    // Переводим сокет в режим прослушивания
    if (listen(clientfd, 5) == -1) {
        std::cerr << "listen() failed: " << std::endl;
        close(clientfd);
        return;
    }
    pthread_mutex_lock(&chanMut);
    busy = 1;
    pthread_mutex_unlock(&chanMut);
    fprintf(stderr, "listen on fd %d\n", clientfd);
    while(!exit) {
        do_message_loop();
    }
}//----------------------------------------------------------------------------------------------------------------------
void VirtualChannel::do_message_loop()
{
    add_wait(outQueue, EPOLLIN);
    add_wait(outCmdQueue, EPOLLIN);
    //отправляем сообщение потоку обработчику, что соединение установлено, mtype = 2
    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(clientfd, 0, CHAN_OPEN_PACKET)), true);
    std::unique_ptr<MessageBuffer> packToSend = nullptr, recvPacket = nullptr, cmdPack = nullptr;
    io_state rcvState = IO_START;
    while (busy)
    {
        shared_ptr<ChanPool> schp = chp.lock();
        if(!schp)
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
    }
    inCmdQueue.push(std::unique_ptr<MessageBuffer>(new MessageBuffer(clientfd, 0, CHAN_CLOSE_PACKET)));
    delete_wait(outQueue);
    delete_wait(outCmdQueue);
    outQueue.stop_and_clear();
    outCmdQueue.stop_and_clear();
    zipflag=0;
}
//----------------------------------------------------------------------------------------------------------------------
