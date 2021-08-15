#ifndef CONFIG_H
#define CONFIG_H
//----------------------------------------------------------------------------------------------------------------------
/*#define MAX_TCP_PORTS_DEFAULT                   2
#define MAX_TCP_CLIENTS_DEFAULT                 2
#define MAX_PROTO_SERVERS_DEFAULT				2
#define MAX_PROTO_CLIENTS_DEFAULT				2
//----------------------------------------------------------------------------------------------------------------------
#define MAX_COM_PORTS_DEFAULT				    2*/
//----------------------------------------------------------------------------------------------------------------------
/*#define DEF_TCP_SERVER_THREAD_PRIORITY     	4
#define DEF_TCP_CLIENT_THREAD_PRIORITY		DEF_TCP_SERVER_THREAD_PRIORITY
#define DEF_PROTO_CLIENT_THREAD_PRIORITY	DEF_TCP_SERVER_THREAD_PRIORITY
#define DEF_PROTO_SERVER_THREAD_PRIORITY	DEF_TCP_SERVER_THREAD_PRIORITY
#define DEF_COM_PORT_THREAD_PRIORITY	   	4
#define MIN_ALLOWED_PRIORITY               	1
#define MAX_ALLOWED_PRIORITY               	99*/
//----------------------------------------------------------------------------------------------------------------------
#define UART_MAX_EVENTS								3
//----------------------------------------------------------------------------------------------------------------------
//#define MAX_PACKET_LEN		4128	//4096+32

//----------------------------------------------------------------------------------------------------------------------
/*typedef struct __attribute__((__packed__)) _Packet_MSG_t
{
    int32_t		msg_type;
    uint16_t	packet_len;
    int32_t		fd;
    uint32_t	moreToFollow;
    uint8_t		packetbuff[MAX_PACKET_LEN];
}Packet_MSG_t;*/
//----------------------------------------------------------------------------------------------------------------------
#endif/*CONFIG_H*/
