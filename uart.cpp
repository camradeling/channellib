#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
//----------------------------------------------------------------------------------------------------------------------
#include <sys/socket.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
//----------------------------------------------------------------------------------------------------------------------
#include "uart.h"
#include "chanpool.h"
//----------------------------------------------------------------------------------------------------------------------
void COMPort::thread_run()
{
    busy = 1;
    while(!exit)
        do_message_loop();
}
//----------------------------------------------------------------------------------------------------------------------
int COMPort::init()
{
    tcgetattr(clientfd, &Config);
    config_com();
    Config.c_cc[VMIN] = 0;
    Config.c_cc[VTIME] = 0;
    tcsetattr(clientfd, TCSANOW, &Config);
    return BasicChannel::init();
}
//----------------------------------------------------------------------------------------------------------------------
int COMPort::send_packet(MessageBuffer *packet, enum io_state state, bool zipped)
{
    static uint32_t bytesSent;
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
        int bytesSentLast = com_write_chunk(clientfd, sendBuf, sendBufLen);
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
int COMPort::recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state)
{
    char buffer[1024];
    int bytesRecvdLast = 0;
    string total="";
    do
    {
        bytesRecvdLast = read (clientfd, buffer, 1024);
        if(bytesRecvdLast < 0)
        {
            if (errno == EAGAIN)
            {
                break;
            }
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
int COMPort::com_write_chunk(int fd, char *buf, int nbytes)
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return -1;
    fd_set rdset,wrset,exset; FD_ZERO(&rdset);FD_ZERO(&wrset);FD_ZERO(&exset);
    FD_SET(fd,&wrset);
    timeval_t tmt;
    tmt.tv_sec =1;tmt.tv_usec=0;
    int res = select(fd+1,&rdset,&wrset,&exset, &tmt);
    if(res < 0)
    {
        WRITELOG("%s[%d]: com select error\n",alias.c_str(),clientfd);
    }
    else if(res == 0)
    {
        WRITELOG("%s[%d]: com send packet timeout\n",alias.c_str(),clientfd);
        //ничо не делаем - таймаут
    }
    else
    {
        if (FD_ISSET(fd, &wrset))
        {
            res = write(fd, buf, nbytes);
        }
    }
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
void COMPort::config_com()
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return;
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
    WRITELOG("%s[%d]: configured. speed = %d",alias.c_str(),clientfd, tcfg.SPEED);
}
//----------------------------------------------------------------------------------------------------------------------
