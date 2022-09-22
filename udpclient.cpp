#include <unistd.h>
#include <fcntl.h>
//----------------------------------------------------------------------------------------------------------------------
#include "tcpbase.h"
#include "udpclient.h"
#include "chanpool.h"
//----------------------------------------------------------------------------------------------------------------------
void UdpClientSocket::thread_run()
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
        clientfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(clientfd == -1)
        {
            WRITELOG("%s[%d]: cannot set up socket  - %d", alias.c_str(),clientfd, errno);
            usleep(1000000);
            close(clientfd);
            continue;
        }
        fcntl(clientfd, F_SETFL, O_NONBLOCK);
        if(bindport)
        {
            struct sockaddr_in myaddr;
            myaddr.sin_family    = AF_INET; // IPv4
            myaddr.sin_addr.s_addr = INADDR_ANY;
            myaddr.sin_port = htons(bindport);
            if ( bind(clientfd, (const struct sockaddr *)&myaddr,
                      sizeof(myaddr)) < 0 )
            {
                WRITELOG("%s[%d]: bind failed", alias.c_str(),clientfd);
            }
        }
        busy = 1;
        do_message_loop();
        close(clientfd);
        pthread_mutex_lock(&chanMut);
        busy = 0;
        pthread_mutex_unlock(&chanMut);
        WRITELOG("%s[%d]: connection closed", alias.c_str(),clientfd);
        usleep(3000000);
    }//while not exit
}
//----------------------------------------------------------------------------------------------------------------------
int UdpClientSocket::clear_and_close()
{
    close(clientfd);
    pthread_mutex_lock(&chanMut);
    busy = 0;
    pthread_mutex_unlock(&chanMut);
    exit = 1;
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int UdpClientSocket::send_packet(MessageBuffer *packet, enum io_state state, bool zipped)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    static uint32_t bytesSent;
    uint32_t  packLen = packet->Length();
    uint32_t bytesToSend = packLen;
    if(state == IO_START)
        bytesSent = 0;
    do
    {
        sendfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sendfd == -1)
        {
            WRITELOG("%s[%d]: cannot set up socket  - %d", alias.c_str(),clientfd, errno);
            usleep(1000000);
            close(clientfd);
            return bytesToSend - bytesSent;
        }
        fcntl(sendfd, F_SETFL, O_NONBLOCK);
        char *sendBuf;
        int sendBufLen;
        sendBuf = packet->Data() + bytesSent;
        sendBufLen = packLen - bytesSent;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(peer_addr.sin_addr), str, INET_ADDRSTRLEN);
        int bytesSentLast = sendto(sendfd, (const char *)sendBuf, sendBufLen,
               MSG_CONFIRM | MSG_DONTWAIT, (const struct sockaddr *) &peer_addr, sizeof(peer_addr));
        close(sendfd);
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
int UdpClientSocket::recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state)
{
    char buffer[1024];
    int bytesRecvdLast = 0;
    string total="";
    do
    {
        bytesRecvdLast = recvfrom(clientfd, (char *)buffer, 1024,
                                  MSG_DONTWAIT, (struct sockaddr *) &peer_addr, &peer_addrlen);
        if(bytesRecvdLast < 0)
        {
            if (errno == EAGAIN)
                break;
            return -1;
        }
        total += string(buffer,bytesRecvdLast);
    }
    while(bytesRecvdLast);
    *packet = std::unique_ptr<MessageBuffer>(new MessageBuffer(clientfd, total.length(), CHAN_DATA_PACKET));
    memcpy(packet->get()->Data(), total.data(),total.length());
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------