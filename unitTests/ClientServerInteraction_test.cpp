#include "catch.hpp"

#include <comm/Client.h>
#include <comm/Server.h>
#include <thread>
#include <iostream>

// Port gets tied up after each test, so use a global and increment it
// so that each test uses a different port
size_t portNumber=9008;

SCENARIO( "Test that the Client and Server can interact properly", "[integration]" )
{
	GIVEN( "A Client and server" )
	{
		comm::Server myServer;
		comm::Client myClient;

		WHEN( "I start a server listening and try and connect a client to it" )
		{
			REQUIRE_NOTHROW( myServer.listen( ++portNumber ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(portNumber) ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			REQUIRE_NOTHROW( myClient.disconnect() );
			REQUIRE_NOTHROW( myServer.stop() );
		}
		WHEN( "I send info messages from a client to a server" )
		{
			std::string concatenatedInfoMessages; // Store all received info message in here
			REQUIRE_NOTHROW( myServer.setDefaultInfoHandler( [&](const std::string& message){ concatenatedInfoMessages+=message; } ) );
			std::string concatenatedRequestMessages; // Store all received info message in here
			REQUIRE_NOTHROW( myServer.setDefaultRequestHandler( [&](const std::string& message){ concatenatedRequestMessages+=message; return "Answer is: "+message; } ) );

			std::cout << "Starting to listen" << std::endl;
			REQUIRE_NOTHROW( myServer.listen( ++portNumber ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(portNumber) ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			myClient.sendInfo( "This is the first test" );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );
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

			std::cout << "Starting to listen" << std::endl;
			REQUIRE_NOTHROW( myServer.listen( ++portNumber ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(portNumber) ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );


			for( auto& pConnection : myServer.currentConnections() ) pConnection->sendInfo( "This is the first test" );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) ); // Allow 1/2 a second for the message to arrive
			CHECK( concatenatedInfoMessages=="This is the first test" );
			for( auto& pConnection : myServer.currentConnections() ) pConnection->sendInfo( "This is the second test" );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) ); // Allow 1/2 a second for the message to arrive
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

			std::cout << "Starting to listen" << std::endl;
			REQUIRE_NOTHROW( myServer.listen( ++portNumber ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			REQUIRE_NOTHROW( myClient.connect( "ws://localhost:"+std::to_string(portNumber) ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			myClient.sendRequest( "This is the first test", [&](const std::string& message){concatenatedResponses+=message;} );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );
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
			std::vector<comm::Client> clients;
			for( size_t index=0; index<numberOfClients; ++index ) clients.emplace_back();

			REQUIRE_NOTHROW( myServer.listen( ++portNumber ) );
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );

			// Connect all of the clients, and check that the server sees them
			for( size_t index=0; index<clients.size(); ++index )
			{
				REQUIRE_NOTHROW( clients[index].connect( "ws://localhost:"+std::to_string(portNumber) ) );
				std::this_thread::sleep_for( std::chrono::milliseconds(500) );
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
