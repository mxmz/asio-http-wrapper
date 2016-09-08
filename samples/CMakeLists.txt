PROJECT(samples)
cmake_minimum_required(VERSION 2.4)

ADD_DEFINITIONS(-D_REENTRANT )
INCLUDE_DIRECTORIES( . ./include/ ../include)
LINK_DIRECTORIES( ${LINK_DIRECTORIES} )	


ADD_EXECUTABLE ( buffpipe1 buffpipe1.cpp )
ADD_EXECUTABLE ( ring_buffer_test ring_buffer_test.cpp)



TARGET_LINK_LIBRARIES( buffpipe1  -lboost_system )
TARGET_LINK_LIBRARIES( ring_buffer_test  -lboost_system -pthread )



SET_TARGET_PROPERTIES( buffpipe1 ring_buffer_test
		PROPERTIES 
		COMPILE_FLAGS "-g -fpermissive -Wall -std=c++11 -std=c++14"
)
