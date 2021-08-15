#ifndef UDPCLIENT_H
#define UDPCLIENT_H
//----------------------------------------------------------------------------------------------------------------------
#include "ifaddrs.h"
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
//----------------------------------------------------------------------------------------------------------------------
#include "basicchannel.h"
#include "chanpool.h"
#include "tcpbase.h"
//----------------------------------------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
class UdpClientSocket : public TcpBase
{
public:
    UdpClientSocket(shared_ptr<ChanPool> chp):TcpBase(chp) {chanType="UDPClient";}
    virtual ~UdpClientSocket(){}
public:
    virtual void thread_run();
    virtual int clear_and_close();
    virtual int send_packet(MessageBuffer *packet, enum io_state state, bool zipped=0);
    virtual int recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state);
    uint16_t bindport;
    int 	sendfd;
};
//---------------------------------------------------------------------------------------------
#endif/*TCPCLIENT*/