#include "tcpbase.h"
#include "chanpool.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
//----------------------------------------------------------------------------------------------------------------------
void TcpBase::enable_keepalive(int sock)
{
    shared_ptr<ChanPool> schanpool = chanpool.lock();
    if(!schanpool)
        return;
    int yes = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) < 0)
    {
        WRITELOG("%s[%d]: setsockopt(SO_KEEPALIVE) failed, error %d",alias.c_str(),clientfd,errno);
    }
    int idle = 10;
    if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) < 0)
    {
        WRITELOG("%s[%d]: setsockopt(TCP_KEEPIDLE) failed, error %d",alias.c_str(),clientfd,errno);
    }
    int interval = 1;
    if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int)) < 0)
    {
        WRITELOG("%s[%d]: setsockopt(TCP_KEEPINTVL failed, error %d",alias.c_str(),clientfd,errno);
    }
    int maxpkt = 3;
    if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int)) < 0)
    {
        WRITELOG("%s[%d]: setsockopt(TCP_KEEPCNT) failed, error %d",alias.c_str(),clientfd,errno);
    }
}
//----------------------------------------------------------------------------------------------------------------------