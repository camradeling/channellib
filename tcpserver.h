#ifndef TCPSERVER_H
#define TCPSERVER_H
//----------------------------------------------------------------------------------------------------------------------
#include <map>
#include "tcpbase.h"
//----------------------------------------------------------------------------------------------------------------------
#define MAX_TCP_CLIENTS 4
//----------------------------------------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
class TcpPeerSocket;
//----------------------------------------------------------------------------------------------------------------------
class TcpServerSocket : public TcpBase
{
public:
	TcpServerSocket(shared_ptr<ChanPool> chp): TcpBase(chp) {chanType="TcpServer";}
	virtual ~TcpServerSocket();
	int32_t			    listfd=-1;
	struct sockaddr_in	listaddr;
    int MaxPeers = MAX_TCP_CLIENTS;
	map<int,shared_ptr<TcpPeerSocket>> peers;
    virtual shared_ptr<TcpPeerSocket> new_peer();
public:
	virtual void thread_run();
    virtual void do_message_loop();
    virtual int clear_and_close();
	
};
//----------------------------------------------------------------------------------------------------------------------
class TcpPeerSocket : public TcpBase
{
public:    
    TcpPeerSocket(shared_ptr<ChanPool> chp):TcpBase(chp){alias="TcpPeer";}
    virtual ~TcpPeerSocket();
    virtual void thread_run(){}
    string peeraddr="";
    io_state rcvState = IO_START;
    std::unique_ptr<MessageBuffer> packToSend = nullptr;
    std::unique_ptr<MessageBuffer> recvPacket = nullptr;
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*TCPSERVER_H*/
