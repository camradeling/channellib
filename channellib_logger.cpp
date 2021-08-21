#include <stdio.h>
#include <cstdarg>
//----------------------------------------------------------------------------------------------------------------------
#include "channellib_logger.h"
//----------------------------------------------------------------------------------------------------------------------
int ChannelLib::Logger::write( int debugCode, const char * format, ... )
{
	va_list args;
    va_start(args, format);
	fprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	return 0;
}
//----------------------------------------------------------------------------------------------------------------------