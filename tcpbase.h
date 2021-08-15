#ifndef BAULBASE_H
#define BAULBASE_H
//----------------------------------------------------------------------------------------------------------------------
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
//----------------------------------------------------------------------------------------------------------------------
#include "basicchannel.h"
//----------------------------------------------------------------------------------------------------------------------
class TcpBase : public BasicChannel
{
public:
    TcpBase(shared_ptr<ChanPool> chp): BasicChannel(chp) {chanType="TcpBase";alias="TcpBase";}
    virtual ~TcpBase(){}
    struct sockaddr_in peer_addr;
    socklen_t peer_addrlen=sizeof(struct sockaddr_in);
protected:
    virtual void thread_run() = 0;
    virtual void enable_keepalive(int sock);
};
//----------------------------------------------------------------------------------------------------------------------
#endif //BAULBASE_H
