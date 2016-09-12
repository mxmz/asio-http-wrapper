PROJECT(samples)
cmake_minimum_required(VERSION 2.4)

ADD_DEFINITIONS(-D_REENTRANT )
INCLUDE_DIRECTORIES( . ./include/ ../include)
LINK_DIRECTORIES( ${LINK_DIRECTORIES} )	


ADD_EXECUTABLE ( buffpipe1                  buffpipe1.cpp )
ADD_EXECUTABLE ( ring_buffer_test           ring_buffer_test.cpp)
ADD_EXECUTABLE ( stream_pipe_test           stream_pipe_test.cpp)
ADD_EXECUTABLE ( http_parser_test           http_parser_test.cpp ../http-parser/http_parser.c)
ADD_EXECUTABLE ( http_inbound_connection_test   http_inbound_connection_test.cpp ../http-parser/http_parser.c)


TARGET_LINK_LIBRARIES( buffpipe1                    -lboost_system )
TARGET_LINK_LIBRARIES( ring_buffer_test             -lboost_system -pthread )
TARGET_LINK_LIBRARIES( http_parser_test             -lboost_system -pthread )
TARGET_LINK_LIBRARIES( stream_pipe_test             -lboost_system -pthread )
TARGET_LINK_LIBRARIES( http_inbound_connection_test     -lboost_system -pthread )



SET_TARGET_PROPERTIES(  buffpipe1 
                        ring_buffer_test 
                        http_parser_test 
                        stream_pipe_test 
                        http_inbound_connection_test
		PROPERTIES 
		COMPILE_FLAGS "-g -fpermissive -Wall -std=c++11 -std=c++14"
)
