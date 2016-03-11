#include "catch.hpp"

#include <communique/Client.h>
#include <communique/Server.h>
#include <thread>
#include <iostream>
#include <list>
#include <mutex>

#include "testinputs.h"

SCENARIO( "Test that the Client and Server can interact properly", "[integration][local]" )
{

	GIVEN( "A Client and server" )
	{
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );

		communique::Client myClient;
		myClient.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );

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


			for( auto& pConnection : myServer.currentConnections() ) pConnection.lock()->sendInfo( "This is the first test" );
			std::this_thread::sleep_for( testinputs::shortWait ); // Allow 1/2 a second for the message to arrive
			CHECK( concatenatedInfoMessages=="This is the first test" );
			for( auto& pConnection : myServer.currentConnections() ) pConnection.lock()->sendInfo( "This is the second test" );
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
	}
}

SCENARIO( "Test that a Server can handle multiple connections", "[integration][local]" )
{
	GIVEN( "A server" )
	{
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );

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
				std::this_thread::sleep_for( testinputs::shortWait );
				CHECK( myServer.currentConnections().size()==clients.size()-index-1 );
			}
		}
	}
}

SCENARIO( "Test that the Server can communicate with two clients correctly", "[integration][local][custom]" )
{
	GIVEN( "A server and two clients" )
	{
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );

		communique::Client client1;
		communique::Client client2;
		client1.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );
		client2.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );

		WHEN( "When I send info messages to all connected clients" )
		{
			// Record all info messages received by the clients somewhere
			std::stringstream client1Info; // All the info messages received by client1
			std::stringstream client2Info; // Dito client2
			REQUIRE_NOTHROW( client1.setInfoHandler( [&](const std::string& message){ client1Info << message << '\n'; } ) );
			REQUIRE_NOTHROW( client2.setInfoHandler( [&](const std::string& message){ client2Info << message << '\n'; } ) );

			// Start the server listening and connect both clients
			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );
			REQUIRE_NOTHROW( client1.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			REQUIRE_NOTHROW( client2.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			// These block until the connection completes, so it's useful for synchronisation
			CHECK( client1.isConnected() );
			CHECK( client2.isConnected() );

			// Send some arbitrary information to all connected clients
			std::vector<std::weak_ptr<communique::IConnection> > connections;
			REQUIRE_NOTHROW( connections=myServer.currentConnections() );
			for( const auto& connection : connections )
			{
				REQUIRE_NOTHROW( connection.lock()->sendInfo("This is message 1") );
			}
			for( const auto& connection : connections )
			{
				REQUIRE_NOTHROW( connection.lock()->sendInfo("This is message 2") );
			}

			// Disconnect one of the clients and do the same again
			std::this_thread::sleep_for( testinputs::shortWait ); // Make sure the messages have had time to be received
			client1.disconnect();
			std::this_thread::sleep_for( testinputs::shortWait ); // Make sure the server has finished processing the disconnection

			// First try and use the old list of connections before updating. Since it uses weak_ptr
			// this should be fine as long as we check first.
			for( const auto& connection : connections )
			{
				auto pConnection=connection.lock();
				if( pConnection )
				{
					REQUIRE_NOTHROW( pConnection->sendInfo("This is message 3") );
				}
			}

			// Also try with an updated list of connections, without checking the lock was successful
			REQUIRE_NOTHROW( connections=myServer.currentConnections() );
			for( const auto& connection : connections )
			{
				REQUIRE_NOTHROW( connection.lock()->sendInfo("This is message 4") );
			}

			std::this_thread::sleep_for( testinputs::shortWait ); // Make sure the messages have had time to be received
			CHECK( client1Info.str() == "This is message 1\nThis is message 2\n" );
			CHECK( client2Info.str() == "This is message 1\nThis is message 2\nThis is message 3\nThis is message 4\n" );

			CHECK_NOTHROW( client2.disconnect() ); // Disconnecting the clients first frees up the port quicker (apparently)
			std::this_thread::sleep_for( testinputs::shortWait );
		}
	}
	GIVEN( "A subscription server and some clients" )
	{
		// This is to replicate a subscribe and publish type scenario, where a client connects and
		// asks to subscribe. The server then sends periodic information to all subscribed clients.
		// After a client disconnects, the server should correctly handle an unsuccessful sendInfo
		// attempt.
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );
		// Set up the request handlers that allow client to subscribe and unsubscribe
		std::list<std::weak_ptr<communique::IConnection> > subscribedClients;
		std::mutex subscribedClientsMutex;
		auto serverRequestHandler=[&](const std::string& message,std::weak_ptr<communique::IConnection> connection)->std::string
			{
				if( message=="subscribe" )
				{
					std::lock_guard<std::mutex> lock(subscribedClientsMutex);
					subscribedClients.push_back(connection);
					return "ok";
				}
				else if( message=="unsubscribe" )
				{
					std::lock_guard<std::mutex> lock(subscribedClientsMutex);
					// Can't use std::find because weak_ptrs cannot be compared. Have to lock and then compare.
					auto pThisConnection=connection.lock();
					auto iFindResult=std::find_if( subscribedClients.begin(), subscribedClients.end(), [&](std::weak_ptr<communique::IConnection> element){return element.lock()==pThisConnection;} );
					if( iFindResult==subscribedClients.end() ) return "client not subscribed";
					else
					{
						subscribedClients.erase(iFindResult);
						return "ok";
					}
				}
				else return "Unknown request";
			};
		REQUIRE_NOTHROW( myServer.setDefaultRequestHandler(serverRequestHandler) );
		// This next lambda is not a handler, just a convenience function to send messages to subscribers
		auto sendMessageToSubscribers=[&](const std::string& message)
			{
				std::lock_guard<std::mutex> lock(subscribedClientsMutex);
				auto iSubscriber=subscribedClients.begin();
				while( iSubscriber!=subscribedClients.end() ) // Can't use a for loop because I'm deleting iterators inside the loop
				{
					auto pConnection=iSubscriber->lock();
					if( pConnection ) REQUIRE_NOTHROW( pConnection->sendInfo(message) );
					else subscribedClients.erase(iSubscriber--); // Connection must have closed, so unsubscribe. Post decrement because iSubscriber will be invalidated by the erase.
					++iSubscriber;
				}
			};

		//
		// Now create the clients
		//
		communique::Client client1;
		communique::Client client2;
		client1.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );
		client2.setVerifyFile( testinputs::testFileDirectory+"certificateAuthority_cert.pem" );
		// Set up info handlers for the clients to handle the subscribed events
		std::stringstream client1SubscribedEvents;
		std::stringstream client2SubscribedEvents;
		REQUIRE_NOTHROW( client1.setInfoHandler([&](const std::string& message){client1SubscribedEvents << message;}) );
		REQUIRE_NOTHROW( client2.setInfoHandler([&](const std::string& message){client2SubscribedEvents << message;}) );

		WHEN( "When I unsubscribe a client but don't disconnect it" )
		{

			// Start the server listening and connect both clients
			REQUIRE_NOTHROW( myServer.listen( ++testinputs::portNumber ) );
			std::this_thread::sleep_for( testinputs::shortWait );
			REQUIRE_NOTHROW( client1.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			REQUIRE_NOTHROW( client2.connect( "ws://localhost:"+std::to_string(testinputs::portNumber) ) );
			// These block until the connection completes, so it's useful for synchronisation
			CHECK( client1.isConnected() );
			CHECK( client2.isConnected() );

			// Subscribe both clients
			std::string client1Response;
			std::string client2Response;
			REQUIRE_NOTHROW( client1.sendRequest( "subscribe", [&](const std::string& message){client1Response=message;} ) );
			REQUIRE_NOTHROW( client2.sendRequest( "subscribe", [&](const std::string& message){client2Response=message;} ) );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( client1Response=="ok" );
			CHECK( client2Response=="ok" );
			CHECK( subscribedClients.size()==2 );

			// Send the first subscription message from the server
			REQUIRE_NOTHROW( sendMessageToSubscribers("1") );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( client1SubscribedEvents.str()=="1" );
			CHECK( client2SubscribedEvents.str()=="1" );

			// Do this a couple more times
			REQUIRE_NOTHROW( sendMessageToSubscribers("2") );
			REQUIRE_NOTHROW( sendMessageToSubscribers("3") );
			REQUIRE_NOTHROW( sendMessageToSubscribers("4") );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( client1SubscribedEvents.str()=="1234" );
			CHECK( client2SubscribedEvents.str()=="1234" );

			// Unsubscribe one client (but don't disconnect) and make sure the send message deals with out okay
			REQUIRE_NOTHROW( client1.sendRequest( "unsubscribe", [&](const std::string& message){client1Response=message;} ) );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( client1Response=="ok" );

			// Send some more message
			REQUIRE_NOTHROW( sendMessageToSubscribers("5") );
			REQUIRE_NOTHROW( sendMessageToSubscribers("6") );
			REQUIRE_NOTHROW( sendMessageToSubscribers("7") );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( client1SubscribedEvents.str()=="1234" );
			CHECK( client2SubscribedEvents.str()=="1234567" );

			// Just for the hell of it, make sure the server handles a double unsubscribe
			REQUIRE_NOTHROW( client1.sendRequest( "unsubscribe", [&](const std::string& message){client1Response=message;} ) );
			std::this_thread::sleep_for( testinputs::shortWait );
			CHECK( client1Response=="client not subscribed" );

			CHECK_NOTHROW( client1.disconnect() );
			CHECK_NOTHROW( client2.disconnect() );
			// Allowing the clients to fully disconnect before the server frees up the port more quickly
			std::this_thread::sleep_for( testinputs::shortWait );
		}
	}
}
