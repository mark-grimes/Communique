#
# CMake file to build Communique.
# Mark Grimes (kknb1056@gmail.com)
# 31/Oct/2014
#

PROJECT( Communique )

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 0 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 0 )


ADD_DEFINITIONS( "-std=c++11" )

#
# Download the source for WebSocket++ from github. Don't install it since it's
# header only, just add the downloaded directory to the include path. 
#
include(ExternalProject)
ExternalProject_Add( websocketpp
	GIT_REPOSITORY https://github.com/mark-grimes/websocketpp.git
	GIT_TAG addOriginalAsioError
	CONFIGURE_COMMAND "" # Just using the source directory as an include directory
	BUILD_COMMAND "" # Just using the source directory as an include directory
	INSTALL_COMMAND "" # Don't want to install, it's header only
)
ExternalProject_Get_Property( websocketpp SOURCE_DIR )
INCLUDE_DIRECTORIES( "${SOURCE_DIR}" )

#
# Find and add boost to the include directories
#
SET( Boost_USE_MULTITHREADED )
SET( Boost_USE_STATIC_LIBS ON )
FIND_PACKAGE( Boost REQUIRED COMPONENTS system )
#IF( NOT Boost_FOUND )
#	MESSAGE( FATAL_ERROR "Boost wasn't found. If boost is in a non standard path try adding "
#		"\"-DCMAKE_INCLUDE_PATH=<path to boost>\" and \"-DCMAKE_LIBRARY_PATH=<path to boost>\""
#		"to the cmake command"
#	)
#ENDIF()
Message( "Boost include directory = ${Boost_INCLUDE_DIR}" )
Message( "Boost libraries directory = ${Boost_LIBRARY_DIR}" )
Message( "Boost libraries directory = ${Boost_LIBRARIES}" )
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

#
# Find OpenSSL
#
FIND_PACKAGE( OpenSSL REQUIRED )
INCLUDE_DIRECTORIES( ${OPENSSL_INCLUDE_DIR} )

Message( "OPENSSL_LIBRARIES = ${OPENSSL_LIBRARIES}" )

INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/include" )
INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/privateinclude" )
AUX_SOURCE_DIRECTORY( "${CMAKE_SOURCE_DIR}/src" library_sources )


ADD_LIBRARY( ${PROJECT_NAME} SHARED ${library_sources} )
ADD_DEPENDENCIES( ${PROJECT_NAME} websocketpp ) # Make sure WebSocket++ is downloaded before trying to build
target_link_libraries( ${PROJECT_NAME} ${Boost_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} ${OPENSSL_LIBRARIES} )

install( TARGETS ${PROJECT_NAME} DESTINATION lib )
install( DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION include )

#
# Create the CommuniqueConfig.cmake file so that other projects
# can use the library.
#
get_property( CONF_LIBRARIES TARGET ${PROJECT_NAME} PROPERTY LOCATION )
set( CONF_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include" )
configure_file( ${PROJECT_NAME}Config.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY )

OPTION( BUILD_UNITTESTS "Build unit tests" ON )
MESSAGE( STATUS "BUILD_UNITTESTS: ${BUILD_UNITTESTS}" )
IF( BUILD_UNITTESTS )
	# Fix the test configuration file to have the correct paths
	CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/test/testinputs.cpp.in" "${PROJECT_SOURCE_DIR}/test/testinputs.cpp" @ONLY )
	AUX_SOURCE_DIRECTORY( "test" unittests_sources )
	AUX_SOURCE_DIRECTORY( "test/impl" unittests_sources )
	ADD_EXECUTABLE( unitTests.exe ${unittests_sources} )
	TARGET_LINK_LIBRARIES( unitTests.exe ${PROJECT_NAME} )
ENDIF()
