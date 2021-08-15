#ifndef TCPCLIENT_H
#define TCPCLIENT_H
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
class TcpClientSocket : public TcpBase
{
public:
	TcpClientSocket(shared_ptr<ChanPool> chp):TcpBase(chp) {chanType="TcpClient";}
	virtual ~TcpClientSocket();
public:
	virtual void thread_run();
	virtual int clear_and_close();
};
//---------------------------------------------------------------------------------------------
#endif/*TCPCLIENT*/
