#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
//----------------------------------------------------------------------------------------------------------------------
#include "protoclient.h"
#include "chanpool.h"
//----------------------------------------------------------------------------------------------------------------------
int ProtoClientSocket::recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state)
{
    static int32_t bytesRecvd;
    static int32_t packLen;
    if(state == IO_START)
    {
        bytesRecvd = 0;
        packLen = 0;
    }
    int32_t bytesToRecv = sizeof(uint32_t) + (bytesRecvd >= sizeof(uint32_t)) ? packLen : 0;
    do
    {
        char *recvBuf;
        int recvBufLen;
        if(bytesRecvd < sizeof(uint32_t))
        {
            recvBuf = (char *)&packLen + bytesRecvd;
            recvBufLen = sizeof(uint32_t) - bytesRecvd;
        }
        else
        {
            recvBuf = packet->get()->Data() + (bytesRecvd - sizeof(uint32_t));
            recvBufLen = packLen - (bytesRecvd - sizeof(uint32_t));
        }

        int bytesRecvdLast = recv(clientfd, recvBuf, recvBufLen, 0);
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
        bytesRecvd += bytesRecvdLast;
        if(bytesRecvd == sizeof(uint32_t))
        {
            int zipped=0;
            if(packLen & (1 << 31))
            {
                packLen &= ~(1 << 31);
                zipflag=1;
                zipped=1;
            }
            if(packLen > (1024*1024))
            {
                return -1;
            }
            *packet = std::unique_ptr<MessageBuffer>(new MessageBuffer(clientfd, packLen, ((zipped)?CHAN_DATA_PACKET_ZIPPED:CHAN_DATA_PACKET)));
        }
        bytesToRecv = sizeof(uint32_t) + packLen;
    } while(bytesRecvd < bytesToRecv);
    return bytesToRecv - bytesRecvd;
}
//----------------------------------------------------------------------------------------------------------------------
int ProtoClientSocket::send_packet(MessageBuffer *packet, enum io_state state, bool zipped)
{
    static uint32_t bytesSent;
    uint32_t  packLen = packet->Length();
    uint32_t bytesToSend = packLen + sizeof(uint32_t);
    if(zipflag && zipped)
        packLen |= (1 << 31);
    if(state == IO_START)
        bytesSent = 0;
    do
    {
        char *sendBuf;
        int sendBufLen;
        if (bytesSent < sizeof(uint32_t))
        {
            sendBuf = (char *)&packLen + bytesSent;
            sendBufLen = sizeof(uint32_t) - bytesSent;
        }
        else
        {
            sendBuf = packet->Data() + (bytesSent - sizeof(uint32_t));
            sendBufLen = (packLen & ~(1 << 31)) - (bytesSent - sizeof(uint32_t));
        }
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