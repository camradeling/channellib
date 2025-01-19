#include "MessageBuffer.h"
//----------------------------------------------------------------------------------------------------------------------
MessageBuffer::MessageBuffer(uint32_t cfd, size_t length, enum MessageType type, string chaddr)
        : data(length), msgType(type), fd(cfd)
{
}
//----------------------------------------------------------------------------------------------------------------------
MessageBuffer::MessageBuffer(MessageBuffer* src): data(src->Length()), msgType(src->Type()), fd(src->getfd())
{
	data = src->data;
	seqnum = src->seqnum;
}
//----------------------------------------------------------------------------------------------------------------------