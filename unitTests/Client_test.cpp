#include "catch.hpp"

#include <communique/Client.h>
#include <thread>
#include <iostream>

SCENARIO( "Test that Client behaves as expected" )
{
	GIVEN( "A Client" )
	{
		communique::Client myClient;

		WHEN( "I try to connect to ws://echo.websocket.org" )
		{
			REQUIRE_NOTHROW( myClient.connect( "ws://echo.websocket.org" ) );
			std::this_thread::sleep_for( std::chrono::seconds(3) );
			REQUIRE_NOTHROW( myClient.disconnect() );
		}
//		WHEN( "I try to connect to a non-existent address" )
//		{
//			REQUIRE_THROWS( myClient.connect( "ws://blah.blahasdf.com" ) );
//		}
	}
}
