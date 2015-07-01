#include "catch.hpp"

#include <communique/Client.h>
#include <communique/Server.h>
#include <thread>
#include <iostream>

#include "testinputs.h"

SCENARIO( "Test that the Client and Server can interact properly", "[integration]" )
{
	GIVEN( "A Client and server" )
	{
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"old/server.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"old/server.pem" );

		communique::Client myClient;
		myClient.setVerifyFile( testinputs::testFileDirectory+"old/rootCA.pem" );

		WHEN( "I start a server listening and try and connect a client to it" )
		{
			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE( myClient.isConnected() );
			REQUIRE( myServer.currentConnections().size()==1 );

			REQUIRE_NOTHROW( myClient.disconnect() );
			REQUIRE_NOTHROW( myServer.stop() );
		}
		WHEN( "I send info messages from a client to a server" )
		{
			std::string concatenatedInfoMessages; // Store all received info message in here
			REQUIRE_NOTHROW( myServer.setDefaultInfoHandler( [&](const std::string& message){ concatenatedInfoMessages+=message; } ) );
			std::string concatenatedRequestMessages; // Store all received info message in here
			REQUIRE_NOTHROW( myServer.setDefaultRequestHandler( [&](const std::string& message){ concatenatedRequestMessages+=message; return "Answer is: "+message; } ) );

			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			myClient.sendInfo( "This is the first test" );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( concatenatedInfoMessages=="This is the first test" );
			CHECK( concatenatedRequestMessages=="" );

			concatenatedInfoMessages.clear();

			REQUIRE_NOTHROW( myClient.disconnect() );
			std::this_thread::sleep_for( std::chrono::seconds(2) );
			REQUIRE_NOTHROW( myServer.stop() );
		}
		WHEN( "I send info messages from a server to a client" )
		{
			std::string concatenatedInfoMessages; // Store all received info message in here
			REQUIRE_NOTHROW( myClient.setInfoHandler( [&](const std::string& message){ concatenatedInfoMessages+=message; } ) );
			std::string concatenatedRequestMessages; // Store all received info message in here
			REQUIRE_NOTHROW( myClient.setRequestHandler( [&](const std::string& message){ concatenatedRequestMessages+=message; return "Answer is: "+message; } ) );

			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			std::this_thread::sleep_for( testinputs::shortWait );


			for( auto& pConnection : myServer.currentConnections() ) pConnection->sendInfo( "This is the first test" );
			std::this_thread::sleep_for( testinputs::shortWait ); // Allow 1/2 a second for the message to arrive
			CHECK( concatenatedInfoMessages=="This is the first test" );
			for( auto& pConnection : myServer.currentConnections() ) pConnection->sendInfo( "This is the second test" );
			std::this_thread::sleep_for( testinputs::shortWait ); // Allow 1/2 a second for the message to arrive
			CHECK( concatenatedInfoMessages=="This is the first testThis is the second test" );
			CHECK( concatenatedRequestMessages=="" );

			concatenatedInfoMessages.clear();

			// Always disconnect the client first, otherwise the port can get tied up.
			REQUIRE_NOTHROW( myClient.disconnect() );
			std::this_thread::sleep_for( std::chrono::seconds(2) );
			REQUIRE_NOTHROW( myServer.stop() );
		}
		WHEN( "I send request messages from a client to a server" )
		{
			std::string concatenatedResponses; // Store all received info message in here
			std::string concatenatedRequestMessages; // Store all received info message in here
			REQUIRE_NOTHROW( myServer.setDefaultRequestHandler( [&](const std::string& message){ concatenatedRequestMessages+=message; return "Answer is: "+message; } ) );

			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			myClient.sendRequest( "This is the first test", [&](const std::string& message){concatenatedResponses+=message;} );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( concatenatedResponses=="Answer is: This is the first test" );
			CHECK( concatenatedRequestMessages=="This is the first test" );

			REQUIRE_NOTHROW( myClient.disconnect() );
			std::this_thread::sleep_for( std::chrono::seconds(2) );
			REQUIRE_NOTHROW( myServer.stop() );
		}
		WHEN( "I connect and disconnect several clients to a server" )
		{
			const size_t numberOfClients=5;

			// Create several clients
			std::vector<communique::Client> clients;
			for( size_t index=0; index<numberOfClients; ++index ) clients.emplace_back();

			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );

			// Connect all of the clients, and check that the server sees them
			for( size_t index=0; index<clients.size(); ++index )
			{
				REQUIRE_NOTHROW( clients[index].connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
				std::this_thread::sleep_for( testinputs::shortWait );
				CHECK( myServer.currentConnections().size()==index+1 );
			}

			// disconnect all the clients, and check that the server realises they're disconnected
			for( size_t index=0; index<clients.size(); ++index )
			{
				REQUIRE_NOTHROW( clients[index].disconnect() );
				CHECK( myServer.currentConnections().size()==clients.size()-index-1 );
			}
		}
	}
}
