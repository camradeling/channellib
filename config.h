#ifndef CONFIG_H
#define CONFIG_H
//----------------------------------------------------------------------------------------------------------------------
#include <string>
//----------------------------------------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------------------------------------
#define UART_MAX_EVENTS								3
//----------------------------------------------------------------------------------------------------------------------
enum COMParity
{
	COM_EDN_PARITY_NONE=0,
	COM_EDN_PARITY_EVEN,
	COM_EDN_PARITY_ODD
};
//----------------------------------------------------------------------------------------------------------------------
typedef struct _UARTConfig_t
{
    uint32_t		SPEED;
    uint16_t		WORDLEN;
    COMParity		EDN;
    uint16_t		STOPBITS;
    uint16_t		HARDFLOW;
}UARTConfig_t;
//----------------------------------------------------------------------------------------------------------------------
typedef struct COMInitStrust_t
{
	string alias="";
	string device="";
	string function="";
	UARTConfig_t uconf;
}COMInitStruct;
//----------------------------------------------------------------------------------------------------------------------
typedef struct TCPServerInitStruct_t
{
	string alias="";
	string function="";
	uint16_t listen_port=0;
}TCPServerInitStruct;
//----------------------------------------------------------------------------------------------------------------------
typedef struct ProtoServerInitStruct_t
{
	string alias="";
	string function="";
	uint16_t listen_port=0;
	uint16_t keep_alive_sec=0;
}ProtoServerInitStruct;
//----------------------------------------------------------------------------------------------------------------------
typedef struct TCPClientInitStruct_t
{
	string alias="";
	string function="";
	string peeraddr;
	uint16_t port=0;
}TCPClientInitStruct;
//----------------------------------------------------------------------------------------------------------------------
typedef struct ProtoClientInitStruct_t
{
	string alias="";
	string function="";
	string peeraddr="";
	uint16_t port=0;
	uint16_t keep_alive_sec=0;
}ProtoClientInitStruct;
//----------------------------------------------------------------------------------------------------------------------
typedef struct UDPClientInitStruct_t
{
	string alias="";
	string function="";
	string peeraddr;
	uint16_t port=0;
	uint16_t bindport=0;
}UDPClientInitStruct;
//----------------------------------------------------------------------------------------------------------------------
class ChanPoolConfig
{
public:
	ChanPoolConfig(){}
	~ChanPoolConfig(){}
	vector<COMInitStruct> allCOMPorts;
	vector<TCPServerInitStruct> allTCPServers;
	vector<ProtoServerInitStruct> allProtoServers;
	vector<TCPClientInitStruct> allTCPClients;
	vector<ProtoClientInitStruct> allProtoClients;
	vector<UDPClientInitStruct> allUDPClients;
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*CONFIG_H*/
