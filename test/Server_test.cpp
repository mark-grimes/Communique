#include "catch.hpp"

#include <communique/Server.h>
#include <deque>
#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "testinputs.h"

// The "[.]" stops this test being run as part of the main suite, since it requires
// user intervention to fire up a browser and point it to the required address.
SCENARIO( "Test that a Server can handle connections from a webbrowser", "[.][manual]" )
{
	GIVEN( "A Server" )
	{
		communique::Server myServer;
		myServer.setCertificateChainFile( testinputs::testFileDirectory+"server_cert.pem" );
		myServer.setPrivateKeyFile( testinputs::testFileDirectory+"server_key.pem" );

		WHEN( "Serving files from a directory" )
		{
			// Synchronisation variables to know when the test is over and the server should stop listening.
			bool testIsOver=false;
			std::mutex testIsOverMutex;
			std::condition_variable testIsOverCondition;

			// This is a FIFO queue of how the conversation is expected to go. The first string is
			// the expected request from the client, and the second is the required reply the server
			// should make. Once a request-response exchange has taken place that entry is taken off
			// the front before the next request arrives.
			std::deque<std::pair<std::string,std::string> > expectedConversation={
				{"Hello there","Why hello, nice to see you."},
				{"Nothing else to say","Well disconnect then!"} };
			auto requestHandler=[&](const std::string& message)->std::string
				{
					if( !expectedConversation.empty() )
					{
						if( expectedConversation.front().first == message )
						{
							std::string appropriateResponse=expectedConversation.front().second;
							expectedConversation.pop_front();
							if( expectedConversation.empty() )
							{
								std::lock_guard<std::mutex> lock(testIsOverMutex);
								testIsOver=true;
								std::cout << "Finishing test" << std::endl;
								testIsOverCondition.notify_all();
							}
							return appropriateResponse;
						}
						else return "What? You're supposed to say '"+expectedConversation.front().second+"'";
					}
					else return "Not expecting any further communication";
				};
			REQUIRE_NOTHROW( myServer.setDefaultRequestHandler(requestHandler) );
			REQUIRE_NOTHROW( myServer.setFileServeRoot(testinputs::testFileDirectory+"www") );
			REQUIRE_NOTHROW( myServer.listen( testinputs::browserTestPortNumber ) );
			std::cout << "Broadcasting files on port " << testinputs::browserTestPortNumber << ". Fire up a browser and point it to:" << "\n"
					<< "     https://localhost:" << testinputs::browserTestPortNumber << "/serverTest.html" << "\n"
					<< "The connection MUST be https and the certificate is self signed, so you will probably have to agree to a security exception."
					<< std::endl;

			std::unique_lock<std::mutex> lock(testIsOverMutex);
			while( !testIsOver ) testIsOverCondition.wait_for( lock, std::chrono::seconds(60) );
			//std::this_thread::sleep_for( testinputs::shortWait ); // Give a little time for the client to disconnect
		}
	}
}
