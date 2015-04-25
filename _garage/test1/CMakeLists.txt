PROJECT(test)
cmake_minimum_required(VERSION 2.4)
SET( test_SRCs 
                test1.cpp
                ../../http-parser/http_parser.c
 )


ADD_DEFINITIONS(-D_REENTRANT )
INCLUDE_DIRECTORIES( . ./include/ ../../http-parser )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} )	


ADD_EXECUTABLE ( test ${test_SRCs} )


TARGET_LINK_LIBRARIES( test  -lboost_system
) 

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 ")

SET_TARGET_PROPERTIES( test
		PROPERTIES 
		OUTPUT_NAME test
		COMPILE_FLAGS "-g -Wall -DPREFIX=\\\"${CMAKE_INSTALL_PREFIX}\\\" "
)


#INSTALL(TARGETS test
#       RUNTIME DESTINATION bin
#       PERMISSIONS WORLD_EXECUTE WORLD_READ SETUID SETGID GROUP_READ OWNER_READ GROUP_EXECUTE OWNER_EXECUTE
#       
#       )
#
#INSTALL (   FILES test.config.json 
#            DESTINATION share/test
#            PERMISSIONS OWNER_READ        
#                
#        )
#

