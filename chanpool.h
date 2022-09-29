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
#include "config.h"
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
	int init(ChanPoolConfig* config);
    //int init_priorities(mxml_node_t* channode);
	int init_tcp(vector<TCPServerInitStruct> tcps);
	int init_tcp_clients(vector<TCPClientInitStruct> tcpc);
	int init_proto_tcp(vector<ProtoServerInitStruct> protos);
	int init_proto_tcp_clients(vector<ProtoClientInitStruct> protoc);
	int init_com_ports(vector<COMInitStruct> coms);
	int init_udp_clients(vector<UDPClientInitStruct> udpc);
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*CHANPOOL_H*/
