/** @file Tests to make sure that invalid certificates fail to connect */
#include "catch.hpp"

#include <communique/Client.h>
#include <communique/Server.h>
#include <thread>
#include <iostream>

#include "testinputs.h"

SCENARIO( "Test that server setup fails when given invalid security files", "[security][local]" )
{
	WHEN( "I create a server with invalid certificate file" )
	{
		communique::Server myServer;
		REQUIRE_NOTHROW( myServer.setCertificateChainFile( testinputs::testFileDirectory+"blahblahblah.pem" ) );
		REQUIRE_THROWS( myServer.listen( ++testinputs::portNumber ) );
	}

	WHEN( "I create a server with invalid key file" )
	{
		communique::Server myServer;
		REQUIRE_NOTHROW( myServer.setPrivateKeyFile( testinputs::testFileDirectory+"blahblahblah.pem" ) );
		REQUIRE_THROWS( myServer.listen( ++testinputs::portNumber ) );
	}

	WHEN( "I create a server with key in place of the certificate" )
	{
		communique::Server myServer;
		REQUIRE_NOTHROW( myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_key.pem" ) );
		REQUIRE_THROWS( myServer.listen( ++testinputs::portNumber ) );
	}

	WHEN( "I create a server with certificate in place of the key" )
	{
		communique::Server myServer;
		REQUIRE_NOTHROW( myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_cert.pem" ) );
		REQUIRE_THROWS( myServer.listen( ++testinputs::portNumber ) );
	}
}

SCENARIO( "Test that client setup fails when given invalid security files", "[security][local]" )
{
	WHEN( "I create a client with invalid verification file" )
	{
		communique::Client myClient;
		REQUIRE_NOTHROW( myClient.setVerifyFile( testinputs::testFileDirectory+"blahblahblah.pem" ) );
		REQUIRE_THROWS( myClient.connect( "wss://echo.websocket.org" ) );
	}
}

SCENARIO( "Test that an incorrect server certificate fails ", "[security][local]" )
{
	GIVEN( "A Client and server" )
	{
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"old/server.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"old/server.pem" );

		communique::Client myClient;
		myClient.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );

		WHEN( "I start a server listening and try and connect a client to it" )
		{
			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );

			REQUIRE( !myClient.isConnected() );

			REQUIRE_NOTHROW( myServer.stop() );
		}
	}
}

SCENARIO( "Test that connection fails when server requires client authentication, and client doesn't authenticate", "[security][local]" )
{
	GIVEN( "A Client and server" )
	{
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );
		myServer.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );

		communique::Client myClient;
		myClient.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );

		WHEN( "I start a server listening and try and connect a client to it" )
		{
			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );

			REQUIRE( !myClient.isConnected() );

			REQUIRE_NOTHROW( myServer.stop() );
		}
	}
}

SCENARIO( "Test that client authentication works", "[security][local]" )
{
	GIVEN( "A Client and server" )
	{
		communique::Server myServer;
		myServer.setErrorLogLevel(0xffffffff);
		myServer.setErrorLogLocation( std::cerr );
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );
		myServer.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );

		communique::Client myClient;
		myClient.setErrorLogLevel(0xffffffff);
		myClient.setErrorLogLocation( std::cerr );
		myClient.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myClient.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );
		myClient.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );

		WHEN( "I start a server listening and try and connect a client to it" )
		{
			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );

			REQUIRE( myClient.isConnected() );

			REQUIRE_NOTHROW( myClient.disconnect() );
			REQUIRE_NOTHROW( myServer.stop() );
		}
	}
}
