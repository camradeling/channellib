#ifndef BASIC_CHANNEL_H
#define BASIC_CHANNEL_H
//----------------------------------------------------------------------------------------------------------------------
#include <sys/epoll.h>
//----------------------------------------------------------------------------------------------------------------------
#include "MessageBuffer.h"
#include "MessageQueue.h"
//----------------------------------------------------------------------------------------------------------------------
#define WRITELOG(format,...) {if(schanpool->logger) schanpool->logger->write(schanpool->DebugValue,format,##__VA_ARGS__);}
#define WRITELOGEXTRA(format,...) {if(schanpool->logger && schanpool->ExtraLog) schanpool->logger->write(schanpool->DebugValue,format,##__VA_ARGS__);}
//----------------------------------------------------------------------------------------------------------------------
class ChanPool;
//----------------------------------------------------------------------------------------------------------------------
class BasicChannel
{
public:
    BasicChannel(shared_ptr<ChanPool> chp)
            : chanpool(chp), inQueue(false), outQueue(false), inCmdQueue(false), outCmdQueue(false) {}
    virtual ~BasicChannel();
public:
    weak_ptr<ChanPool>			chanpool;
    string              chanType="Undefined";
    MessageQueue<std::unique_ptr<MessageBuffer>> inQueue;
    MessageQueue<std::unique_ptr<MessageBuffer>> outQueue;
    MessageQueue<std::unique_ptr<MessageBuffer>> inCmdQueue;
    MessageQueue<std::unique_ptr<MessageBuffer>> outCmdQueue;
    string              alias="BasicChannel";
    pthread_t           threadID=0;
    pthread_mutex_t     chanMut;
    string				funcName = "";
    uint8_t 			exit = 0;
    uint8_t				busy = 0;
    int 			    clientfd=-1;
    int                 defaultKeepAlive = 0;
    bool                zipflag=0;
    int32_t             bytesRecvd=0;
    int32_t             packLen=0;
    int32_t             bytesSent=0;
    uint32_t            seqnum=0;
public:
    virtual int init();
    virtual void send_message_buffer(MessageQueue<std::unique_ptr<MessageBuffer>>* queue, std::unique_ptr<MessageBuffer> buf, bool forceStart = false) {queue->push(std::move(buf), forceStart);}
    virtual int clear_and_close(){return 0;}
    enum io_state
    {
        IO_START,
        IO_CONTINUE
    };
    virtual int send_packet(MessageBuffer *packet, enum io_state state, bool zipped=0);
    virtual int recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state);
protected:
    int epollFD=-1;
    virtual int start_thread();
    virtual void thread_run() = 0;
    virtual int add_wait(int fd, uint32_t events);
    virtual int delete_wait(int fd);
    virtual int modify_wait(int fd, uint32_t events);
    virtual void do_message_loop();

private:
    static void * _entry_func(void *This) {((BasicChannel *)This)->thread_run(); return NULL;}
};
//----------------------------------------------------------------------------------------------------------------------
#endif/*BASIC_CHANNEL_H*/