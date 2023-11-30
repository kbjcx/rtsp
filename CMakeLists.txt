cmake_minimum_required(VERSION 3.16)
project(RtspServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -o0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -o3 -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(INCLUDE_DIR /usr/local/include)
set(LIB_DIR /usr/local/lib)

include_directories(${INCLUDE_DIR})
link_directories(${LIB_DIR})

set(LIVE 
    ./trunk/live/AACMediaSource.cpp
    ./trunk/live/AACSink.cpp 
    ./trunk/live/Buffer.cpp 
    ./trunk/live/H264MediaSource.cpp 
    ./trunk/live/H264Sink.cpp 
    ./trunk/live/InetAddress.cpp
    ./trunk/live/MediaSource.cpp
    ./trunk/live/MediaSession.cpp
    ./trunk/live/MediaSessionManager.cpp
    ./trunk/live/Rtp.cpp
    ./trunk/live/RtpInstance.cpp
    ./trunk/live/RtspConnection.cpp
    ./trunk/live/RtspServer.cpp
    ./trunk/live/Sink.cpp
    ./trunk/live/TcpConnection.cpp)

set(SCHEDULER 
    ./trunk/scheduler/Event.cpp
    ./trunk/scheduler/EventScheduler.cpp
    ./trunk/scheduler/SelectPoller.cpp
    ./trunk/scheduler/SocketsOptions.cpp
    ./trunk/scheduler/Thread.cpp
    ./trunk/scheduler/ThreadPool.cpp
    ./trunk/scheduler/Timer.cpp
    ./trunk/scheduler/UsageEnvironment.cpp)

add_executable(rtspservrer ${LIVE} ${SCHEDULER} ./trunk/main.cpp)
