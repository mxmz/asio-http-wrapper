 PROJECT(asio1)
cmake_minimum_required(VERSION 2.4)
SET( asio1_SRCs 
        
                3.cpp
 )


ADD_DEFINITIONS(-D_REENTRANT )
INCLUDE_DIRECTORIES( . ./include/ )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} )	


ADD_EXECUTABLE ( asio1 ${asio1_SRCs} )


TARGET_LINK_LIBRARIES( asio1  -lboost_system
) 


SET_TARGET_PROPERTIES( asio1
		PROPERTIES 
		OUTPUT_NAME asio1
		COMPILE_FLAGS "-g -fpermissive -DRELEASE=1 -Wall -std=c++0x -DPREFIX=\\\"${CMAKE_INSTALL_PREFIX}\\\" "
)


#INSTALL(TARGETS asio1
#       RUNTIME DESTINATION bin
#       PERMISSIONS WORLD_EXECUTE WORLD_READ SETUID SETGID GROUP_READ OWNER_READ GROUP_EXECUTE OWNER_EXECUTE
#       
#       )
#
#INSTALL (   FILES asio1.config.json 
#            DESTINATION share/asio1
#            PERMISSIONS OWNER_READ        
#                
#        )
#

