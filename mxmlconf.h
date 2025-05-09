#ifndef MXMLCONF_H
#define MXMLCONF_H
//----------------------------------------------------------------------------------------------------------------------
#include <vector>
#include <stdint.h>
#include <string>
//----------------------------------------------------------------------------------------------------------------------
#include "mxml.h"
#include "config.h"
//----------------------------------------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
vector<COMInitStruct> mxml_parse_com_ports(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
vector<TCPServerInitStruct> mxml_parse_tcp(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
vector<ProtoServerInitStruct> mxml_parse_proto_tcp(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
vector<ProtoClientInitStruct> mxml_parse_proto_tcp_clients(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
vector<TCPClientInitStruct> mxml_parse_tcp_clients(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
vector<UDPClientInitStruct> mxml_parse_udp_clients(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
vector<VirtualInitStruct> mxml_parse_virtual(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
ChanPoolConfig* mxml_parse_config(mxml_node_t* channode);
//----------------------------------------------------------------------------------------------------------------------
#endif/*MXMLCONF_H*/