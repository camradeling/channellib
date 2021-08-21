#include "basicchannel.h"
#include "uart.h"
#include "tcpbase.h"
#include "protoclient.h"
#include "protoserver.h"
#include "udpclient.h"
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init(mxml_node_t* channode)
{
    shared_ptr<ChanPool> schanpool = chp.lock();
    if(!schanpool)
        return -1;
	mxml_node_t* xmlNode, *blockNode;
	if(channode == nullptr)
	{
        WRITELOG("No config provided");
		return 1;
	}
    char* tmpbuff = (char*)mxmlElementGetAttr(channode, "Version");
	if(tmpbuff)
	    configVersion = string(tmpbuff);
    if(configVersion == "2.0")
    {
        blockNode = mxmlFindElement(channode, channode, "CliChannels", NULL, NULL, MXML_DESCEND);
        if(blockNode == nullptr)
        {
            WRITELOG("No <CliChannels> node provided");
            return -1;
        }
        mxml_node_t* extranode = mxmlFindElement(blockNode, blockNode, "ExtraLog", NULL, NULL, MXML_DESCEND);
        if(extranode)
        {
            const char* extxt = mxmlGetText(extranode,NULL);
            if(extxt && string(extxt) == "true") ExtraLog=1;
        }
        init_tcp(blockNode);
        init_tcp_clients(blockNode);
        init_udp_clients(blockNode);
        init_proto_tcp(blockNode);
        init_proto_tcp_clients(blockNode);
        init_com_ports(blockNode);
    }
    else
    {
        WRITELOG("config version mismatch, upgrade to config version 2.0");
    }
    WRITELOG("Channel lib inited");
    inited=1;
	return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_com_ports(mxml_node_t* channode)
{
    shared_ptr<ChanPool> schanpool = chp.lock();
    if(!schanpool)
        return -1;
    mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff = NULL;
    char* tmpport = NULL;
    char* tmpbits=NULL, *tmpparity=NULL, *tmpstop=NULL, *tmpspeed=NULL, *tmppin=NULL, *func=NULL, *hflow=(char*)"NULL";
    for (curnode = mxmlFindElement(channode, channode, "Channel", NULL, NULL, MXML_DESCEND);
         curnode != NULL;
         curnode = mxmlFindElement(curnode, channode, NULL, NULL, NULL, MXML_NO_DESCEND))
    {

        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Type");
        if (tmpbuff == NULL || strcmp(tmpbuff, "COMPort"))
            continue;//for now we process only com channels
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Alias");
        func = (char*)mxmlElementGetAttr(curnode, "Function");
        if(seeknode = mxmlFindElement(curnode, curnode, "Device", NULL, NULL, MXML_DESCEND))
            tmpport = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "Bits", NULL, NULL, MXML_DESCEND))
            tmpbits = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "Parity", NULL, NULL, MXML_DESCEND))
            tmpparity = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "StopBits", NULL, NULL, MXML_DESCEND))
            tmpstop = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "Speed", NULL, NULL, MXML_DESCEND))
            tmpspeed = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "HardwareControl", NULL, NULL, MXML_DESCEND))
            hflow = (char *) mxmlGetText(seeknode, NULL);
        uint16_t val;
        val = atoi(tmpport);
        COMPort* com = new COMPort(schanpool);
        com->alias = string(tmpbuff);
        com->funcName = string(func);
        if(tmpport != NULL)
            com->clientfd = open(tmpport, O_RDWR | O_NOCTTY  | O_EXCL | O_NDELAY);
        if(com->clientfd <= 0)
        {
            WRITELOG("Error open %s", com->alias.c_str());
            continue;
        }
        if(tmpbits != NULL)
            com->tcfg.WORDLEN = atoi(tmpbits);
        if(tmpparity != NULL)
        {
            if(!strcmp(tmpparity, "None"))
                com->tcfg.EDN = COM_EDN_PARITY_NONE;
            else if(!strcmp(tmpparity, "Even"))
                com->tcfg.EDN = COM_EDN_PARITY_EVEN;
            else if(!strcmp(tmpparity, "Odd"))
                com->tcfg.EDN = COM_EDN_PARITY_ODD;
        }
        if(tmpstop != NULL)
            com->tcfg.STOPBITS = atoi(tmpstop);
        if(tmpspeed != NULL)
            com->tcfg.SPEED = atoi(tmpspeed);
        if(hflow != NULL)
        {
            if(!strcasecmp(hflow, "Yes") || !strcasecmp(hflow, "true") || !strcasecmp(hflow, "1"))
                com->tcfg.HARDFLOW = 1;
            else
                com->tcfg.HARDFLOW = 0;
        }
        com->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(com)));
    }
    WRITELOG("com inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_tcp(mxml_node_t* channode)
{
    shared_ptr<ChanPool> schanpool = chp.lock();
    if(!schanpool)
        return -1;
    mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    uint16_t val;
    for (curnode = mxmlFindElement(channode, channode, "Channel", NULL, NULL, MXML_DESCEND);
         curnode != NULL;
         curnode = mxmlFindElement(curnode, channode, NULL, NULL, NULL, MXML_NO_DESCEND))
    {
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Type");
        if (tmpbuff == NULL || strcmp(tmpbuff, "TCP"))
            continue;//for now we process only tcp channels
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Alias");
        char* func = (char*)mxmlElementGetAttr(curnode, "Function");
        char* port = nullptr;
        if(seeknode = mxmlFindElement(curnode, curnode, "ListenPort", NULL, NULL, MXML_DESCEND))
            port = (char *) mxmlGetText(seeknode, NULL);
        if(tmpbuff == NULL || port == NULL || func == NULL)
            continue;
        val = atoi(port);
        TcpServerSocket* tsock = new TcpServerSocket(schanpool);
        tsock->alias = string(tmpbuff);
        tsock->listaddr.sin_family = AF_INET;
        tsock->listaddr.sin_port = htons(val);
        tsock->funcName = string(func);
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("tcp inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_proto_tcp(mxml_node_t* channode)
{
    shared_ptr<ChanPool> schanpool = chp.lock();
    if(!schanpool)
        return -1;
    mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    uint16_t val;
    for (curnode = mxmlFindElement(channode, channode, "Channel", NULL, NULL, MXML_DESCEND);
         curnode != NULL;
         curnode = mxmlFindElement(curnode, channode, NULL, NULL, NULL, MXML_NO_DESCEND))
    {
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Type");
        if (tmpbuff == NULL || strcmp(tmpbuff, "ProtoTCP"))
            continue;//for now we process only tcp channels
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Alias");
        char* func = (char*)mxmlElementGetAttr(curnode, "Function");
        char* port = nullptr;
        char* defKA = nullptr;
        if(seeknode = mxmlFindElement(curnode, curnode, "ListenPort", NULL, NULL, MXML_DESCEND))
            port = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "KeepAlive", NULL, NULL, MXML_DESCEND))
            defKA = (char *) mxmlGetText(seeknode, NULL);
        if(tmpbuff == NULL || port == NULL || func == NULL)
            continue;
        val = atoi(port);
        ProtoServerSocket* tsock = new ProtoServerSocket(schanpool);
        tsock->alias = string(tmpbuff);
        tsock->listaddr.sin_family = AF_INET;
        tsock->listaddr.sin_port = htons(val);
        tsock->funcName = string(func);
        if(defKA)
            tsock->defaultKeepAlive = atoi(defKA);
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("proto tcp inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_proto_tcp_clients(mxml_node_t* channode)
{
    shared_ptr<ChanPool> schanpool = chp.lock();
    if(!schanpool)
        return -1;
    mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    char* tmpport;
    uint16_t val;
    for (curnode = mxmlFindElement(channode, channode, "Channel", NULL, NULL, MXML_DESCEND);
         curnode != NULL;
         curnode = mxmlFindElement(curnode, channode, NULL, NULL, NULL, MXML_NO_DESCEND))
    {
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Type");
        if (tmpbuff == NULL || strcmp(tmpbuff, "ProtoTCPClient"))
            continue;//for now we process only tcp channels
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Alias");
        char* func = (char*)mxmlElementGetAttr(curnode, "Function");

        char* addr = nullptr;
        tmpport = nullptr;
        char* defKA = nullptr;
        if(seeknode = mxmlFindElement(curnode, curnode, "Host", NULL, NULL, MXML_DESCEND))
            addr = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "Port", NULL, NULL, MXML_DESCEND))
            tmpport = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "KeepAlive", NULL, NULL, MXML_DESCEND))
            defKA = (char *) mxmlGetText(seeknode, NULL);
        if(tmpbuff == nullptr || tmpport == nullptr || addr == nullptr)
            continue;
        ProtoClientSocket* tsock = new ProtoClientSocket(schanpool);
        val = atoi(tmpport);
        tsock->alias = string(tmpbuff);
        tsock->funcName = string(func);
        tsock->peer_addr.sin_family = AF_INET;
        tsock->peer_addr.sin_port = htons(val);
        tsock->peer_addr.sin_addr.s_addr = inet_addr(addr);
        tsock->peer_addrlen = sizeof(tsock->peer_addr);
        if(defKA)
            tsock->defaultKeepAlive = atoi(defKA);
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("proto tcp clients inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_tcp_clients(mxml_node_t* channode)
{
    shared_ptr<ChanPool> schanpool = chp.lock();
    if(!schanpool)
        return -1;
    mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    char* tmpport;
    for (curnode = mxmlFindElement(channode, channode, "Channel", NULL, NULL, MXML_DESCEND);
         curnode != NULL;
         curnode = mxmlFindElement(curnode, channode, NULL, NULL, NULL, MXML_NO_DESCEND))
    {
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Type");
        if (tmpbuff == NULL || strcmp(tmpbuff, "TCPClient"))
            continue;//for now we process only telnet channels
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Alias");
        char* func = (char*)mxmlElementGetAttr(curnode, "Function");
        char* addr = nullptr;
        tmpport = nullptr;
        if(seeknode = mxmlFindElement(curnode, curnode, "Host", NULL, NULL, MXML_DESCEND))
            addr = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "Port", NULL, NULL, MXML_DESCEND))
            tmpport = (char *) mxmlGetText(seeknode, NULL);
        if(tmpbuff == nullptr || tmpport == nullptr || addr == nullptr)
            continue;
        TcpClientSocket* tsock = new TcpClientSocket(schanpool);
        uint16_t val;
        val = atoi(tmpport);
        tsock->alias = string(tmpbuff);
        tsock->funcName = string(func);
        tsock->peer_addr.sin_family = AF_INET;
        tsock->peer_addr.sin_port = htons(val);
        tsock->peer_addr.sin_addr.s_addr = inet_addr(addr);
        tsock->peer_addrlen = sizeof(tsock->peer_addr);
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("tcp clients inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ChanPool::init_udp_clients(mxml_node_t* channode)
{
    shared_ptr<ChanPool> schanpool = chp.lock();
    if(!schanpool)
        return -1;
    mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    char* tmpport;
    for (curnode = mxmlFindElement(channode, channode, "Channel", NULL, NULL, MXML_DESCEND);
         curnode != NULL;
         curnode = mxmlFindElement(curnode, channode, NULL, NULL, NULL, MXML_NO_DESCEND))
    {
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Type");
        if (tmpbuff == NULL || strcmp(tmpbuff, "UDPClient"))
            continue;
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Alias");
        char* func = (char*)mxmlElementGetAttr(curnode, "Function");
        char* addr = nullptr;
        tmpport = nullptr;
        char* bindp = nullptr;
        if(seeknode = mxmlFindElement(curnode, curnode, "Host", NULL, NULL, MXML_DESCEND))
            addr = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "Port", NULL, NULL, MXML_DESCEND))
            tmpport = (char *) mxmlGetText(seeknode, NULL);
        if(seeknode = mxmlFindElement(curnode, curnode, "Bind", NULL, NULL, MXML_DESCEND))
            bindp = (char *) mxmlGetText(seeknode, NULL);
        if(tmpbuff == nullptr || tmpport == nullptr || addr == nullptr)
            continue;
        UdpClientSocket* tsock = new UdpClientSocket(schanpool);
        uint16_t val;
        val = atoi(tmpport);
        tsock->alias = string(tmpbuff);
        tsock->funcName = string(func);
        tsock->peer_addr.sin_family = AF_INET;
        tsock->peer_addr.sin_port = htons(val);
        tsock->peer_addr.sin_addr.s_addr = inet_addr(addr);
        tsock->peer_addrlen = sizeof(tsock->peer_addr);
        if(bindp)
        {
            val = atoi(bindp);
            tsock->bindport = val;
        }
        tsock->init();
        allChan.push_back(std::move(shared_ptr<BasicChannel>(tsock)));
    }
    WRITELOG("udp clients inited");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------