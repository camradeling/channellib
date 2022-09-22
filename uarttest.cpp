#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <sys/epoll.h>
//----------------------------------------------------------------------------------------------------------------------
#include <sys/socket.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <termios.h>
#include <string>
#include <memory>
#include "MessageBuffer.h"
//----------------------------------------------------------------------------------------------------------------------
typedef struct _UARTConfig_t
{
    uint32_t		SPEED;
    uint16_t		WORDLEN;
    uint16_t		EDN;
    uint16_t		STOPBITS;
    uint16_t		HARDFLOW;
}UARTConfig_t;
//----------------------------------------------------------------------------------------------------------------------
#define COM_EDN_PARITY_NONE		    0
#define COM_EDN_PARITY_EVEN			1
#define COM_EDN_PARITY_ODD			2
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
string dev = "/dev/ttySAC2";
UARTConfig_t			tcfg;
struct termios  		Config;
uint32_t      			SymTO;
int clientfd = 0;
int epollFD=0;
//----------------------------------------------------------------------------------------------------------------------
void config_com(UARTConfig_t			tcfg)
{
    Config.c_cflag = 0;
    Config.c_iflag = 0;
    Config.c_lflag = 0;
    if (tcfg.SPEED == 2400)
    {
        cfsetispeed (&(Config), B2400);
        cfsetospeed (&(Config), B2400);
        SymTO = (1000000 / 2400) * 50;
    }
    else if (tcfg.SPEED == 4800)
    {
        cfsetispeed (&(Config), B4800);
        cfsetospeed (&(Config), B4800);
        SymTO = (1000000 / 4800) * 50;
    }
    else if (tcfg.SPEED == 9600)
    {
        cfsetispeed (&(Config), B9600);
        cfsetospeed (&(Config), B9600);
        SymTO = (1000000 / 9600) * 50;
    }
    else if (tcfg.SPEED == 19200)
    {
        cfsetispeed (&(Config), B19200);
        cfsetospeed (&(Config), B19200);
        SymTO = (1000000 / 19200) * 50;
    }
    else if (tcfg.SPEED == 38400)
    {
        cfsetispeed (&(Config), B38400);
        cfsetospeed (&(Config), B38400);
        SymTO = (1000000 / 38400) * 50;
    }
    else if (tcfg.SPEED == 57600)
    {
        cfsetispeed (&(Config), B57600);
        cfsetospeed (&(Config), B57600);
        SymTO = (1000000 / 57600) * 50;
    }
    else if (tcfg.SPEED == 115200)
    {
        cfsetispeed (&(Config), B115200);
        cfsetospeed (&(Config), B115200);
        SymTO = (1000000 / 115200) * 50;
    }
    else if (tcfg.SPEED == 230400)
    {
        cfsetispeed (&(Config), B230400);
        cfsetospeed (&(Config), B230400);
        SymTO = (1000000 / 230400) * 50;
    }
    else if (tcfg.SPEED == 1000000)
    {
        cfsetispeed (&(Config), B1000000);
        cfsetospeed (&(Config), B1000000);
        SymTO = 50;
    }
    else
    {
        cfsetispeed (&(Config), B115200);
        cfsetospeed (&(Config), B115200);
        SymTO = (1000000 / 115200) * 50;
    }
    SymTO *= 5;//иначе не хватает, почему - не знаю. По всем расчетам должно было хватать
    if (tcfg.EDN == COM_EDN_PARITY_NONE)       // none
    {
        Config.c_cflag &= ~PARENB;
    }
    else if (tcfg.EDN == COM_EDN_PARITY_EVEN) // Even
    {
        Config.c_cflag |= PARENB;
        Config.c_cflag &= ~PARODD;
    }
    else if (tcfg.EDN == COM_EDN_PARITY_ODD) // odd
    {
        Config.c_cflag |= PARENB;
        Config.c_cflag |= PARODD;
    }
    else
    {
        Config.c_cflag &= ~PARENB;
    }
    if (tcfg.STOPBITS == 2)       // 2 stop
    {
        Config.c_cflag |= CSTOPB;
    }
    else // 1 stop
    {
        Config.c_cflag &= ~CSTOPB;
    }
    if (tcfg.HARDFLOW != 0)       // Hardware flow control
    {
        Config.c_cflag |= CRTSCTS;
    }
    else // No hardware flow control
    {
        Config.c_cflag &= ~CRTSCTS;
    }
    Config.c_cflag |= (CLOCAL | CREAD);
    Config.c_cflag &= ~CSIZE;
    if (tcfg.WORDLEN == 8)
        Config.c_cflag |= CS8;
    else if (tcfg.WORDLEN == 7)
        Config.c_cflag |= CS7;
    else if (tcfg.WORDLEN == 6)
        Config.c_cflag |= CS6;
    else if (tcfg.WORDLEN == 5)
        Config.c_cflag |= CS5;
    else
        Config.c_cflag |= CS8;
    Config.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | INPCK | ISTRIP | PARMRK | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY | IUTF8 | IUCLC);
    Config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    Config.c_oflag &= ~(OPOST);
}
//----------------------------------------------------------------------------------------------------------------------
int recv_packet(std::unique_ptr<MessageBuffer> *packet)
{
    char buffer[1024];
    int bytesRecvdLast = 0;
    string total="";
    do
    {   fprintf(stderr, "try read\n");
        bytesRecvdLast = read(clientfd, buffer, 1024);
        if(bytesRecvdLast < 0)
        {
            fprintf(stderr, "socket read error %d\n",errno);
            if (errno == EAGAIN)
            {
                break;
            }
            return -1;
        }
        else if(bytesRecvdLast == 0)
        {
        	fprintf(stderr, "bytes read %d\n",bytesRecvdLast);
            return -2;
        }
        fprintf(stderr, "bytes read %d\n",bytesRecvdLast);
        total += string(buffer,bytesRecvdLast);
    }
    while(bytesRecvdLast);
    *packet = std::unique_ptr<MessageBuffer>(new MessageBuffer(clientfd, total.length(), CHAN_DATA_PACKET));
    memcpy(packet->get()->Data(), total.data(),total.length());
    return 0;
}   
//----------------------------------------------------------------------------------------------------------------------
int main()
{
	epollFD = epoll_create1(EPOLL_CLOEXEC);
    if (epollFD < 0)
    {
        fprintf(stderr, "epoll_create1 failed\n");
        return 0;
    }
	fprintf(stderr, "Opening COM\n");
    clientfd = open(dev.c_str(), O_RDWR | O_NOCTTY  | O_EXCL | O_NDELAY);
    if(clientfd <= 0)
    {
        fprintf(stderr, "Error open COM\n");
        sleep(1);
        return -1;;
    }
    fprintf(stderr, "COM opened successfully\n");
    tcfg.EDN = COM_EDN_PARITY_NONE;
    tcfg.WORDLEN = 8;
    tcfg.STOPBITS = 1;
    tcfg.SPEED = 115200;
    tcfg.HARDFLOW = 0;
    tcgetattr(clientfd, &Config);
    config_com(tcfg);
    Config.c_cc[VMIN] = 0;
    Config.c_cc[VTIME] = 0;
    tcsetattr(clientfd, TCSANOW, &Config);
    std::unique_ptr<MessageBuffer> recvPacket;
    struct epoll_event epollEv;
    epollEv.data.fd = clientfd;
    epollEv.events = EPOLLIN;
    epoll_ctl(epollFD, EPOLL_CTL_ADD, clientfd, &epollEv);
    fprintf(stderr, "start receiving\n");
    while(1)
    {
        struct epoll_event ev;
        int res = 0;
        //ждем ивента с любого из направлений
        if ((res = epoll_wait(epollFD, &ev, 1, 10)) < 0)
        {
            if (errno == EINTR)
                continue;
            fprintf(stderr,"COM: epoll_wait error\n");
            break;
        }
        if(!res)
            continue;
        if (ev.data.fd == clientfd)
        {
        	if (ev.events & EPOLLIN)
            {
                fprintf(stderr, "socket EPOLLIN event\n");
                //пришли данные из сокета
                int res = recv_packet(&recvPacket);
                if (res < 0)
                {
                    //ошибка или закрыто соединение
                    if(res == -2)
                    {
                       fprintf(stderr, "connection closed by remote peer\n");
                    }
                    else
                    {
                        fprintf(stderr, "receive from socket error\n");
                    }
                    //break;
                    continue;
                }
                else if (res == 0)
                {
                    //пакет полностью принят
                    //inQueue.push(std::move(recvPacket),true);
                    //rcvState = IO_START;
                    fprintf(stderr, "packet arrived, %d bytes\n",recvPacket->Length());
                    recvPacket.reset(nullptr);
                }
                else
                {
                	fprintf(stderr, "continue receiving data\n");
                    //олучили часть пакета, продолжаем прием
                    //rcvState = IO_CONTINUE;
                }
            }
        }
    }
    sleep(1);
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
