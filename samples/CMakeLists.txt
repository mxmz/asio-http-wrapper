PROJECT(samples)
cmake_minimum_required(VERSION 2.4)

ADD_DEFINITIONS(-D_REENTRANT )
INCLUDE_DIRECTORIES( . ./include/ ../include)
LINK_DIRECTORIES( ${LINK_DIRECTORIES} )	


ADD_EXECUTABLE ( fileserver_example                  fileserver.cpp ../http-parser/http_parser.c ../tests/test.cpp )

TARGET_LINK_LIBRARIES( fileserver_example            -lboost_system -pthread -lboost_filesystem )


SET_TARGET_PROPERTIES(  fileserver_example 
		PROPERTIES 
		COMPILE_FLAGS "-g -fpermissive -Wall -std=c++11 -std=c++14 -O3" 
)
