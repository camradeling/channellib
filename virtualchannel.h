#ifndef VIRTUAL_CHANNEL_H
#define VIRTUAL_CHANNEL_H
//----------------------------------------------------------------------------------------------------------------------
#include <functional>
#include "basicchannel.h"
//----------------------------------------------------------------------------------------------------------------------
class VirtualChannel : public BasicChannel
{
public:
    VirtualChannel(shared_ptr<ChanPool> chp): BasicChannel(chp) {chanType="virtual";alias="Virt";}
    virtual ~VirtualChannel(){}
    virtual int send_packet(MessageBuffer *packet, enum io_state state, bool zipped=0);
    virtual int recv_packet(std::unique_ptr<MessageBuffer> *packet, enum io_state state);
    virtual void thread_run();
    virtual void do_message_loop();
    std::function<void(MessageBuffer *packet)> callback = nullptr;
};
//----------------------------------------------------------------------------------------------------------------------
#endif //VIRTUAL_CHANNEL_H
