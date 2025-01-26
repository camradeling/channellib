#include "MessageBuffer.h"
//----------------------------------------------------------------------------------------------------------------------
MessageBuffer::MessageBuffer(uint32_t cfd, size_t length, enum MessageType type)
        : data(length), msgType(type), fd(cfd)
{
}
//----------------------------------------------------------------------------------------------------------------------
MessageBuffer::MessageBuffer(uint32_t cfd, std::vector<uint8_t> input, enum MessageType type)
	: msgType(type), fd(cfd)
{
	data.insert(data.end(), input.data(), input.data()+input.size());
}
//----------------------------------------------------------------------------------------------------------------------
MessageBuffer::MessageBuffer(MessageBuffer* src): data(src->Length()), msgType(src->Type()), fd(src->getfd())
{
	data = src->data;
	seqnum = src->seqnum;
}
//----------------------------------------------------------------------------------------------------------------------