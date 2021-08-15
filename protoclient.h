#ifndef BAULCLIENT_H
#define BAULCLIENT_H
//----------------------------------------------------------------------------------------------------------------------
#include <netinet/ip.h>
#include "ifaddrs.h"
#include "tcpbase.h"
#include "tcpclient.h"
//----------------------------------------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
class ProtoClientSocket : public TcpClientSocket
{
public:
    ProtoClientSocket(shared_ptr<ChanPool> chp):TcpClientSocket(chp) {chanType="ProtoClient";}
	virtual ~ProtoClientSocket(){}
public:
	virtual int send_packet(MessageBuffer *packet, enum io_state state, bool zipped=0);
	virtual int recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state);
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*BAULCLIENT_H*/
