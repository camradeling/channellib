cmake_minimum_required(VERSION 3.6)
project(ChannelLib)
set(CMAKE_CXX_STANDARD 11)
set(PROJECT_NAME ChannelLib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(Sources
        protoclient.cpp
        protoserver.cpp
        basicchannel.cpp
        virtualchannel.cpp
        chanpool.cpp
        tcpbase.cpp
        tcpclient.cpp
        udpclient.cpp
        tcpserver.cpp
        uart.cpp
        MessageBuffer.cpp
        channellib_logger.cpp
        mxmlconf.cpp
        )
add_library(${PROJECT_NAME} SHARED ${Sources}  ${version_file})
add_library(${PROJECT_NAME}_static STATIC ${Sources} ${version_file})
target_link_libraries(${PROJECT_NAME} mxml )
if(BUILD_TESTS)
add_executable(udptest udptest.cpp)
add_executable(uarttest uarttest.cpp MessageBuffer.cpp)
endif()
