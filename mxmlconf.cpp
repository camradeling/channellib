#include <string>
//----------------------------------------------------------------------------------------------------------------------
#include "mxmlconf.h"
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
vector<COMInitStruct> mxml_parse_com_ports(mxml_node_t* channode)
{
	mxml_node_t* curnode;
    mxml_node_t* seeknode;
    vector<COMInitStruct> comvec;
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
        COMInitStruct com;
        com.alias = string(tmpbuff);
        com.function = string(func);
        if(tmpport != NULL)
        {
            com.device=string(tmpport);
        }
        if(tmpbits != NULL)
            com.uconf.WORDLEN = atoi(tmpbits);
        if(tmpparity != NULL)
        {
            if(!strcmp(tmpparity, "None"))
                com.uconf.EDN = COM_EDN_PARITY_NONE;
            else if(!strcmp(tmpparity, "Even"))
                com.uconf.EDN = COM_EDN_PARITY_EVEN;
            else if(!strcmp(tmpparity, "Odd"))
                com.uconf.EDN = COM_EDN_PARITY_ODD;
        }
        if(tmpstop != NULL)
            com.uconf.STOPBITS = atoi(tmpstop);
        if(tmpspeed != NULL)
            com.uconf.SPEED = atoi(tmpspeed);
        if(hflow != NULL)
        {
            if(!strcasecmp(hflow, "Yes") || !strcasecmp(hflow, "true") || !strcasecmp(hflow, "1"))
                com.uconf.HARDFLOW = 1;
            else
                com.uconf.HARDFLOW = 0;
        }
        comvec.push_back(com);
    }
    return comvec;
}
//----------------------------------------------------------------------------------------------------------------------
vector<TCPServerInitStruct> mxml_parse_tcp(mxml_node_t* channode)
{
	mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    uint16_t val;
    vector<TCPServerInitStruct> tcpvec;
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
        TCPServerInitStruct tcp;
        tcp.alias=string(tmpbuff);
        tcp.function = string(func);
        val = atoi(port);
        tcp.listen_port = val;
        tcpvec.push_back(tcp);
    }
    return tcpvec;
}
//----------------------------------------------------------------------------------------------------------------------
vector<ProtoServerInitStruct> mxml_parse_proto_tcp(mxml_node_t* channode)
{
	mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    uint16_t val;
    vector<ProtoServerInitStruct> tcpvec;
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
        ProtoServerInitStruct tcp;
        tcp.alias=string(tmpbuff);
        tcp.function = string(func);
        val = atoi(port);
        tcp.listen_port = val;
        if(defKA)
            tcp.keep_alive_sec = atoi(defKA);
        tcpvec.push_back(tcp);
    }
    return tcpvec;
}
//----------------------------------------------------------------------------------------------------------------------
vector<ProtoClientInitStruct> mxml_parse_proto_tcp_clients(mxml_node_t* channode)
{
	mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    char* tmpport;
    uint16_t val;
    vector<ProtoClientInitStruct> tcpvec;
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
        ProtoClientInitStruct tcp;
        tcp.alias=string(tmpbuff);
        tcp.function = string(func);
        tcp.peeraddr = string(addr);
        val = atoi(tmpport);
        tcp.port = val;
        if(defKA)
            tcp.keep_alive_sec = atoi(defKA);
        tcpvec.push_back(tcp);
    }
    return tcpvec;
}
//----------------------------------------------------------------------------------------------------------------------
vector<TCPClientInitStruct> mxml_parse_tcp_clients(mxml_node_t* channode)
{
	mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    char* tmpport;
    uint16_t val;
    vector<TCPClientInitStruct> tcpvec;
    for (curnode = mxmlFindElement(channode, channode, "Channel", NULL, NULL, MXML_DESCEND);
         curnode != NULL;
         curnode = mxmlFindElement(curnode, channode, NULL, NULL, NULL, MXML_NO_DESCEND))
    {
        tmpbuff = (char*)mxmlElementGetAttr(curnode, "Type");
        if (tmpbuff == NULL || strcmp(tmpbuff, "TCPClient"))
            continue;//for now we process only tcp channels
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
        TCPClientInitStruct tcp;
        tcp.alias=string(tmpbuff);
        tcp.function = string(func);
        tcp.peeraddr = string(addr);
        val = atoi(tmpport);
        tcp.port = val;
        tcpvec.push_back(tcp);
    }
    return tcpvec;
}
//----------------------------------------------------------------------------------------------------------------------
vector<UDPClientInitStruct> mxml_parse_udp_clients(mxml_node_t* channode)
{
	mxml_node_t* curnode;
    mxml_node_t* seeknode;
    char* tmpbuff;
    char* tmpport;
    uint16_t val;
    vector<UDPClientInitStruct> udpvec;
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
        UDPClientInitStruct udp;
        udp.alias=string(tmpbuff);
        udp.function = string(func);
        udp.peeraddr = string(addr);
        val = atoi(tmpport);
        udp.port = val;
        if(bindp)
        {
            val = atoi(bindp);
            udp.bindport = val;
        }
        udpvec.push_back(udp);
    }
    return udpvec;
}
//----------------------------------------------------------------------------------------------------------------------
ChanPoolConfig* mxml_parse_config(mxml_node_t* channode)
{
	mxml_node_t* xmlNode, *blockNode;
	if(channode == nullptr)
		return nullptr;
    blockNode = mxmlFindElement(channode, channode, "CliChannels", NULL, NULL, MXML_DESCEND);
    if(blockNode == nullptr)
        return nullptr;
    ChanPoolConfig* config = new ChanPoolConfig;
    config->allTCPServers = mxml_parse_tcp(blockNode);
    config->allTCPClients = mxml_parse_tcp_clients(blockNode);
    config->allUDPClients = mxml_parse_udp_clients(blockNode);
    config->allProtoServers = mxml_parse_proto_tcp(blockNode);
    config->allProtoClients = mxml_parse_proto_tcp_clients(blockNode);
    config->allCOMPorts = mxml_parse_com_ports(blockNode);
    return config;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
