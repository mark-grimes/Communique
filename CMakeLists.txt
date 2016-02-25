#
# CMake file to build Communique.
# Mark Grimes (kknb1056@gmail.com)
# 31/Oct/2014
#

project( Communique )

cmake_minimum_required(VERSION 2.8)

set( ${PROJECT_NAME}_MAJOR_VERSION 0 )
set( ${PROJECT_NAME}_MINOR_VERSION 0 )
set( ${PROJECT_NAME}_PATCH_LEVEL 0 )

if( NOT MSVC ) # Microsoft Visual Studio is C++11 by default and doesn't recognise this flag
	add_definitions( "-std=c++11" )
endif()

#
# Download the source for WebSocket++ from github. Don't install it since it's
# header only, just add the downloaded directory to the include path. 
#
include(ExternalProject)
externalproject_add( websocketpp
	GIT_REPOSITORY https://github.com/mark-grimes/websocketpp.git
	GIT_TAG addOriginalAsioError
	CONFIGURE_COMMAND "" # Just using the source directory as an include directory
	BUILD_COMMAND "" # Just using the source directory as an include directory
	INSTALL_COMMAND "" # Don't want to install, it's header only
)
externalproject_get_property( websocketpp SOURCE_DIR )
include_directories( "${SOURCE_DIR}" )

if( ASIO_STANDALONE_PATH )
	message( "Attempting to use standalone ASIO instead of boost from = ${ASIO_STANDALONE_PATH}" )
	include_directories( ${ASIO_STANDALONE_PATH}/include )
	# tell websocketpp to use asio standalone, and full C++11 support instead of boost stand-ins
	add_definitions( "-DASIO_STANDALONE -D_WEBSOCKETPP_CPP11_STRICT_" )
else()
	#
	# Find and add boost to the include directories
	#
	set( Boost_USE_MULTITHREADED )
	set( Boost_USE_STATIC_LIBS ON )
	find_package( Boost REQUIRED COMPONENTS system )
	#if( NOT Boost_FOUND )
	#	message( FATAL_ERROR "Boost wasn't found. If boost is in a non standard path try adding "
	#		"\"-DCMAKE_INCLUDE_PATH=<path to boost>\" and \"-DCMAKE_LIBRARY_PATH=<path to boost>\""
	#		"to the cmake command"
	#	)
	#endif()
	message( "Boost include directory = ${Boost_INCLUDE_DIR}" )
	message( "Boost libraries directory = ${Boost_LIBRARY_DIR}" )
	message( "Boost libraries directory = ${Boost_LIBRARIES}" )
	include_directories(${Boost_INCLUDE_DIR})
endif()

#
# Find OpenSSL
#
find_package( OpenSSL REQUIRED )
include_directories( ${OPENSSL_INCLUDE_DIR} )

message( "OPENSSL_LIBRARIES = ${OPENSSL_LIBRARIES}" )

include_directories( "${CMAKE_SOURCE_DIR}/include" )
include_directories( "${CMAKE_SOURCE_DIR}/privateinclude" )
aux_source_directory( "${CMAKE_SOURCE_DIR}/src" library_sources )


add_library( ${PROJECT_NAME} SHARED ${library_sources} )
add_dependencies( ${PROJECT_NAME} websocketpp ) # Make sure WebSocket++ is downloaded before trying to build
target_link_libraries( ${PROJECT_NAME} ${Boost_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} ${OPENSSL_LIBRARIES} )
if( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
	# For some reason linux gets "undefined reference to `pthread_create'" even though
	# the code uses std::thread. Apparently ASIO sets some funny macros which directly
	# call pthreads.
	target_link_libraries( ${PROJECT_NAME} "pthread" )
endif()

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

option( BUILD_UNITTESTS "Build unit tests" ON )
message( STATUS "BUILD_UNITTESTS: ${BUILD_UNITTESTS}" )
if( BUILD_UNITTESTS )
	# Fix the test configuration file to have the correct paths
	configure_file( "${PROJECT_SOURCE_DIR}/test/testinputs.cpp.in" "${PROJECT_SOURCE_DIR}/test/testinputs.cpp" @ONLY )
	aux_source_directory( "test" unittests_sources )
	aux_source_directory( "test/impl" unittests_sources )
	add_executable( unitTests.exe ${unittests_sources} )
	target_link_libraries( unitTests.exe ${PROJECT_NAME} )
endif()
