#ifndef CHANPOOL_H
#define CHANPOOL_H
//----------------------------------------------------------------------------------------------------------------------
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include "channellib_logger.h"
//----------------------------------------------------------------------------------------------------------------------
#include "MessageQueue.h"
#include "MessageBuffer.h"
//----------------------------------------------------------------------------------------------------------------------
#include "mxml.h"
//----------------------------------------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
typedef struct timeval timeval_t;
typedef struct timespec timespec_t;
//----------------------------------------------------------------------------------------------------------------------
class TcpBase;
class TcpServerSocket;
class TcpClientSocket;
class UdpClientSocket;
class ProtoServerSocket;
class ProtoClientSocket;
class COMPort;
class MessageBuffer;
//----------------------------------------------------------------------------------------------------------------------
class ChanPool
{
public:
	ChanPool(ChannelLib::Logger* lgr=nullptr):logger(lgr){}
	virtual ~ChanPool(){}
public:
	weak_ptr<ChanPool> chp;//ссылка на себя, чтоб передавать дальше
    ChannelLib::Logger* logger=nullptr;
    bool ExtraLog=0;
    uint32_t DebugValue = 0;
    string configVersion = "";
    bool inited=false;
public:
	vector<shared_ptr<BasicChannel>> allChan;
public:
	int init(mxml_node_t* channode);
    int init_priorities(mxml_node_t* channode);
	int init_tcp(mxml_node_t* channode);
	int init_tcp_clients(mxml_node_t* channode);
	int init_proto_tcp(mxml_node_t* channode);
	int init_proto_tcp_clients(mxml_node_t* channode);
	int init_com_ports(mxml_node_t* channode);
	int init_udp_clients(mxml_node_t* channode);
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*CHANPOOL_H*/
