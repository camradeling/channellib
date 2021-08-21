#ifndef CHANNELLIB_LOGGER_H
#define CHANNELLIB_LOGGER_H
//----------------------------------------------------------------------------------------------------------------------
namespace ChannelLib
{
//----------------------------------------------------------------------------------------------------------------------
//inherit from this and override write method if you need
//some behavior different from simple fprintf	
class Logger
{
public:
	Logger(){}
	virtual ~Logger(){}
	int write( int debugCode, const char * format, ... );
};
//----------------------------------------------------------------------------------------------------------------------
}
//----------------------------------------------------------------------------------------------------------------------
#endif/*CHANNELLIB_LOGGER_H*/