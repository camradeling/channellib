#ifndef BAULSERVER_H
#define BAULSERVER_H
//---------------------------------------------------------------------------------------------
#include "tcpbase.h"
#include "tcpserver.h"
//---------------------------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------------------------
class ProtoPeerSocket;
//---------------------------------------------------------------------------------------------
class ProtoServerSocket : public TcpServerSocket
{
public:
    ProtoServerSocket(shared_ptr<ChanPool> chp) : TcpServerSocket(chp) {chanType="ProtoServer";}
	virtual ~ProtoServerSocket(){}
	virtual shared_ptr<TcpPeerSocket> new_peer();
public:
};
//----------------------------------------------------------------------------------------------------------------------
class ProtoPeerSocket : public TcpPeerSocket
{
public:    
    ProtoPeerSocket(shared_ptr<ChanPool> chp):TcpPeerSocket(chp){alias="ProtoPeer";}
    virtual ~ProtoPeerSocket(){}
    virtual int send_packet(MessageBuffer *packet, enum io_state state, bool zipped=0);
	virtual int recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state);
};
//---------------------------------------------------------------------------------------------
#endif/*BAULSERVER_H*/
