PROJECT(tests)
cmake_minimum_required(VERSION 2.4)

ADD_DEFINITIONS(-D_REENTRANT )
INCLUDE_DIRECTORIES( . ./include/ ../include)
LINK_DIRECTORIES( ${LINK_DIRECTORIES} )	


#ADD_EXECUTABLE ( buffpipe1_test             buffpipe1.cpp test.cpp)
ADD_EXECUTABLE ( ring_buffer_test           ring_buffer_test.cpp test.cpp)
ADD_EXECUTABLE ( stream_pipe_test           stream_pipe_test.cpp test.cpp)
ADD_EXECUTABLE ( http_parser_test           http_parser_test.cpp test.cpp ../http-parser/http_parser.c )
ADD_EXECUTABLE ( http_inbound_connection_test   http_inbound_connection_test.cpp test.cpp ../http-parser/http_parser.c)


#TARGET_LINK_LIBRARIES( buffpipe1_test               -lboost_system )
TARGET_LINK_LIBRARIES( ring_buffer_test             -lboost_system -pthread )
TARGET_LINK_LIBRARIES( http_parser_test             -lboost_system -pthread )
TARGET_LINK_LIBRARIES( stream_pipe_test             -lboost_system -pthread )
TARGET_LINK_LIBRARIES( http_inbound_connection_test     -lboost_system -pthread )



SET_TARGET_PROPERTIES(  
                        ring_buffer_test 
                        http_parser_test 
                        stream_pipe_test 
                        http_inbound_connection_test
		PROPERTIES 
		COMPILE_FLAGS "-g -fpermissive -Wall -std=c++11 -std=c++14 -O3 " 
)

ENABLE_TESTING()

ADD_TEST( NAME http_parser_test COMMAND http_parser_test )
ADD_TEST( NAME stream_pipe_test COMMAND stream_pipe_test )
ADD_TEST( NAME ring_buffer_test COMMAND ring_buffer_test )
ADD_TEST( NAME http_inbound_connection_test COMMAND http_inbound_connection_test ENVIRONMENT )

#ADD_TEST( NAME runtests COMMAND bash runtests.sh )

ADD_CUSTOM_COMMAND( OUTPUT run_test.sh 
                    COMMAND ls  ./*_test > run_test.sh
                    DEPENDS http_inbound_connection_test ring_buffer_test stream_pipe_test http_parser_test
                    COMMENT "Generate run_test.sh"
                    )

ADD_CUSTOM_TARGET( RunGenerator DEPENDS run_test.sh
                   COMMENT "Checking if re-generation is required" )

