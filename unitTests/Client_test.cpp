#include "catch.hpp"

#include <communique/Client.h>
#include <thread>
#include <iostream>


SCENARIO( "Test that Client behaves as expected" )
{
	GIVEN( "A Client" )
	{
		communique::Client myClient;

		WHEN( "I try to use a connection before it is connected" )
		{
			REQUIRE( myClient.isConnected()==false );
			REQUIRE_THROWS( myClient.sendInfo( "Blah, blah, blah" ) );
		}
		WHEN( "I try to connect to ws://echo.websocket.org" )
		{
			REQUIRE_NOTHROW( myClient.connect( "ws://echo.websocket.org" ) );
			REQUIRE( myClient.isConnected() );
			REQUIRE_NOTHROW( myClient.disconnect() );
		}
		WHEN( "I try to connect to the secured version wss://echo.websocket.org" )
		{
			REQUIRE_NOTHROW( myClient.connect( "wss://echo.websocket.org" ) );
			REQUIRE( myClient.isConnected() );
			REQUIRE_NOTHROW( myClient.disconnect() );
		}
		WHEN( "I try to connect to a non-existent address" )
		{
			// Asking for a connection should not cause a problem, because the
			// method returns before the handshaking is complete.
			REQUIRE_NOTHROW( myClient.connect( "ws://blah.blahasdf.com" ) );
			// This call blocks until handshaking is complete
			REQUIRE( myClient.isConnected()==false );
			// This call should have no effect
			REQUIRE_NOTHROW( myClient.disconnect() );
		}
	}
}
