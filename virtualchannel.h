#ifndef VIRTUAL_CHANNEL_H
#define VIRTUAL_CHANNEL_H
//----------------------------------------------------------------------------------------------------------------------
#include "basicchannel.h"
//----------------------------------------------------------------------------------------------------------------------
class VirtualChannel : public BasicChannel
{
public:
    VirtualChannel(shared_ptr<ChanPool> chp): BasicChannel(chp) {chanType="virtual";alias="Virt";}
    virtual ~VirtualChannel(){}
    virtual int init() { return 0; }
};
//----------------------------------------------------------------------------------------------------------------------
#endif //VIRTUAL_CHANNEL_H
