#include <unistd.h>
#include <fcntl.h>
//----------------------------------------------------------------------------------------------------------------------
#include "tcpbase.h"
#include "tcpclient.h"
#include "chanpool.h"
//----------------------------------------------------------------------------------------------------------------------
void TcpClientSocket::thread_run()
{
	shared_ptr<ChanPool> schp = chp.lock();
    if(!schp)
        return;
	sigset_t signal_mask;
	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGSEGV);
	pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
	while(!exit)
	{
		clientfd = socket(AF_INET, SOCK_STREAM, 0);
		if(clientfd == -1)
		{
			WRITELOG("%s[%d]:cannot set up socket: %d", alias.c_str(),clientfd, errno);
			usleep(1000000);
			close(clientfd);
			continue;
		}
		int val = 1;
		if (setsockopt(clientfd, SOL_SOCKET, SO_REUSEADDR | SO_KEEPALIVE, &val, sizeof(int)) < 0)
		{
			WRITELOG("%s[%d]: setsockopt(SO_REUSEADDR) failed", alias.c_str(),clientfd);
			usleep(1000000);
			close(clientfd);
			continue;
		}
		fcntl(clientfd, F_SETFL, O_NONBLOCK);
		connect(clientfd , (struct sockaddr *)&peer_addr , sizeof(peer_addr));
		fd_set rdset,wrset,exset;
		FD_ZERO(&wrset);
		FD_SET(clientfd, &wrset);
		timeval_t tv;
		tv.tv_sec = 10;             /* 10 second timeout */
		tv.tv_usec = 0;
		int connected = 0; int res;
		if ((res = select(clientfd + 1, NULL, &wrset, NULL, &tv)) == 1)
		{
			int so_error;
			socklen_t len = sizeof so_error;
			getsockopt(clientfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
			if (so_error == 0)
			{
				connected = 1;
			}
		}
		if(!connected)
		{
			WRITELOG("%s[%d]: clientfd connect failed", alias.c_str(),clientfd);
			close(clientfd);
			usleep(1000000);
			continue;
		}
		else
		{
			pthread_mutex_lock(&chanMut);
			busy = 1;
			pthread_mutex_unlock(&chanMut);
			WRITELOG("%s[%d]: tcp connection established", alias.c_str(),clientfd);
		}
		enable_keepalive(clientfd);
		do_message_loop();
		close(clientfd);
		pthread_mutex_lock(&chanMut);
		busy = 0;
		pthread_mutex_unlock(&chanMut);
		WRITELOG("%s[%d]: connection closed", alias.c_str(),clientfd);
		usleep(3000000);
	}//while not exit
}
//----------------------------------------------------------------------------------------------------------------------
TcpClientSocket::~TcpClientSocket()
{
	exit=1;
	clear_and_close();
}
//----------------------------------------------------------------------------------------------------------------------
int TcpClientSocket::clear_and_close()
{
	pthread_mutex_lock(&chanMut);
    busy = 0;
    pthread_mutex_unlock(&chanMut);
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------