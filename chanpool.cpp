#include "basicchannel.h"
#include "uart.h"
#include "tcpbase.h"
#include "protoclient.h"
#include "protoserver.h"
#include "udpclient.h"
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init(ChanPoolConfig* config)
{
    shared_ptr<ChanPool> schp = chp.lock();
	init_tcp(config->allTCPServers);
    init_tcp_clients(config->allTCPClients);
    init_udp_clients(config->allUDPClients);
    init_proto_tcp(config->allProtoServers);
    init_proto_tcp_clients(config->allProtoClients);
    init_com_ports(config->allCOMPorts);
    WRITELOG("Channel lib inited");
    inited=1;
	return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_com_ports(vector<COMInitStruct> coms)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    for(int i = 0; i < coms.size(); i++)
    {
        COMInitStruct* pis = &coms[i];
        COMPort* com = new COMPort(schp);
        com->alias = pis->alias;
        com->funcName = pis->function;
        com->dev=pis->device;
        com->tcfg=pis->uconf;
        com->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(com)));
    }
    WRITELOG("com inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_tcp(vector<TCPServerInitStruct> tcps)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    for(int i = 0; i < tcps.size(); i++)
    {
        TCPServerInitStruct* pis = &tcps[i];
        TcpServerSocket* tsock = new TcpServerSocket(schp);
        tsock->alias = pis->alias;
        tsock->listaddr.sin_family = AF_INET;
        tsock->listaddr.sin_port = htons(pis->listen_port);
        tsock->funcName = pis->function;
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("tcp inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_proto_tcp(vector<ProtoServerInitStruct> protos)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    for(int i = 0; i < protos.size(); i++)
    {
        ProtoServerInitStruct* pis = &protos[i];
        ProtoServerSocket* tsock = new ProtoServerSocket(schp);
        tsock->alias = pis->alias;
        tsock->listaddr.sin_family = AF_INET;
        tsock->listaddr.sin_port = htons(pis->listen_port);
        tsock->funcName = pis->function;
        tsock->defaultKeepAlive = pis->keep_alive_sec;
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("proto tcp inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_proto_tcp_clients(vector<ProtoClientInitStruct> protoc)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    for(int i = 0; i < protoc.size(); i++)
    {
        ProtoClientInitStruct* pis = &protoc[i];
        ProtoClientSocket* tsock = new ProtoClientSocket(schp);
        tsock->alias = pis->alias;
        tsock->peer_addr.sin_family = AF_INET;
        tsock->peer_addr.sin_port = htons(pis->port);
        tsock->peer_addr.sin_addr.s_addr = inet_addr(pis->peeraddr.c_str());
        tsock->peer_addrlen = sizeof(tsock->peer_addr);
        tsock->funcName = pis->function;
        tsock->defaultKeepAlive = pis->keep_alive_sec;
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("proto tcp clients inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_tcp_clients(vector<TCPClientInitStruct> tcpc)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    for(int i = 0; i < tcpc.size(); i++)
    {
        TCPClientInitStruct* pis = &tcpc[i];
        TcpClientSocket* tsock = new TcpClientSocket(schp);
        tsock->alias = pis->alias;
        tsock->peer_addr.sin_family = AF_INET;
        tsock->peer_addr.sin_port = htons(pis->port);
        tsock->peer_addr.sin_addr.s_addr = inet_addr(pis->peeraddr.c_str());
        tsock->peer_addrlen = sizeof(tsock->peer_addr);
        tsock->funcName = pis->function;
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("tcp clients inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_udp_clients(vector<UDPClientInitStruct> udpc)
{
    shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return -1;
    for(int i = 0; i < udpc.size(); i++)
    {
        UDPClientInitStruct* pis = &udpc[i];
        UdpClientSocket* tsock = new UdpClientSocket(schp);
        tsock->alias = pis->alias;
        tsock->peer_addr.sin_family = AF_INET;
        tsock->peer_addr.sin_port = htons(pis->port);
        tsock->peer_addr.sin_addr.s_addr = inet_addr(pis->peeraddr.c_str());
        tsock->peer_addrlen = sizeof(tsock->peer_addr);
        tsock->bindport = pis->bindport;
        tsock->funcName = pis->function;
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("udp clients inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------