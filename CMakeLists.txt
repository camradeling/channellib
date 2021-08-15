cmake_minimum_required(VERSION 3.6)
project(ChannelLib)

set(CMAKE_CXX_STANDARD 11)

set(PROJECT_NAME ChannelLib)

set(Sources
        protoclient.cpp
        protoserver.cpp
        basicchannel.cpp
        chanpool.cpp
        tcpbase.cpp
        tcpclient.cpp
        udpclient.cpp
        tcpserver.cpp
        uart.cpp
        MessageBuffer.cpp
        )

add_library(${PROJECT_NAME} SHARED ${Sources}  ${version_file})
target_link_libraries(${PROJECT_NAME} mxml )
if(BUILD_TESTS)
add_executable(udptest udptest.cpp)
endif()