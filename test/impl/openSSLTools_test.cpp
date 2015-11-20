#include <communique/impl/openSSLTools.h>
#include "../catch.hpp"

SCENARIO( "Test that the checkHostname function works as expected", "[local][tools][openssl]" )
{
	GIVEN( "A list of allowed hosts contain wildcard entries" )
	{
		std::vector<std::string> allowedHosts;
		allowedHosts.push_back("www.google.com");
		allowedHosts.push_back("*.google.co.uk"); // N.B. bare "google.co.uk" should not match against this
		allowedHosts.push_back("*.google.jp");
		allowedHosts.push_back("google.jp"); // explicitly allow bare hostname
		allowedHosts.push_back("*.appengine.google.com");

		WHEN( "I check for a simple match" )
		{
			CHECK( communique::impl::checkHostname("www.google.com",allowedHosts)==true );
			CHECK( communique::impl::checkHostname("www.microsoft.com",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("asdfaslk",allowedHosts)==false );
		}
		WHEN( "I check for a wildcard match" )
		{
			CHECK( communique::impl::checkHostname("mail.google.com",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("mail.google.co.uk",allowedHosts)==true );
			CHECK( communique::impl::checkHostname("mail.microsoft.co.uk",allowedHosts)==false );
		}
		WHEN( "I check bare hostnames against a wildcard match" )
		{
			CHECK( communique::impl::checkHostname("google.com",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("google.co.uk",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("google.jp",allowedHosts)==true );
			CHECK( communique::impl::checkHostname("microsoft.com",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("microsoft.co.uk",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("microsoft.jp",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("appengine.google.com",allowedHosts)==false );
			CHECK( communique::impl::checkHostname("blah.appengine.google.com",allowedHosts)==true );
		}
	}
}

/* This function was removed because it breaks gcc less than 5 compatible. I'll re-enable the tests once I rewrite the code without std::regex
SCENARIO( "Test that the hostnameFromURL function works as expected", "[local][tools][openssl]" )
{
	WHEN( "I check different URLs for the hostname 'google.com'" )
	{
		CHECK( communique::impl::hostnameFromURL("https://google.com")=="google.com" );
		CHECK( communique::impl::hostnameFromURL("https://google.com/foo/bar")=="google.com" );
		CHECK( communique::impl::hostnameFromURL("https://google.com:443/foo/bar")=="google.com" );
		CHECK( communique::impl::hostnameFromURL("http://google.com:443/foo/bar")=="google.com" );
		CHECK( communique::impl::hostnameFromURL("google.com/foo/bar")=="google.com" );
		CHECK( communique::impl::hostnameFromURL("google.com:443/foo/bar")=="google.com" );
		CHECK( communique::impl::hostnameFromURL("google.com:443")=="google.com" );
		CHECK( communique::impl::hostnameFromURL("google.com")=="google.com" );
	}
	WHEN( "I check different URLs for the hostname 'www.google.com'" )
	{
		CHECK( communique::impl::hostnameFromURL("https://www.google.com")=="www.google.com" );
		CHECK( communique::impl::hostnameFromURL("https://www.google.com/foo/bar")=="www.google.com" );
		CHECK( communique::impl::hostnameFromURL("https://www.google.com:443/foo/bar")=="www.google.com" );
		CHECK( communique::impl::hostnameFromURL("http://www.google.com:443/foo/bar")=="www.google.com" );
		CHECK( communique::impl::hostnameFromURL("www.google.com/foo/bar")=="www.google.com" );
		CHECK( communique::impl::hostnameFromURL("www.google.com:443/foo/bar")=="www.google.com" );
		CHECK( communique::impl::hostnameFromURL("www.google.com:443")=="www.google.com" );
		CHECK( communique::impl::hostnameFromURL("www.google.com")=="www.google.com" );
	}
	WHEN( "I check different URLs for the hostname 'blah.example-with-hyphen.net'" )
	{
		CHECK( communique::impl::hostnameFromURL("https://blah.example-with-hyphen.net")=="blah.example-with-hyphen.net" );
		CHECK( communique::impl::hostnameFromURL("https://blah.example-with-hyphen.net/foo/bar")=="blah.example-with-hyphen.net" );
		CHECK( communique::impl::hostnameFromURL("https://blah.example-with-hyphen.net:443/foo/bar")=="blah.example-with-hyphen.net" );
		CHECK( communique::impl::hostnameFromURL("http://blah.example-with-hyphen.net:443/foo/bar")=="blah.example-with-hyphen.net" );
		CHECK( communique::impl::hostnameFromURL("blah.example-with-hyphen.net/foo/bar")=="blah.example-with-hyphen.net" );
		CHECK( communique::impl::hostnameFromURL("blah.example-with-hyphen.net:443/foo/bar")=="blah.example-with-hyphen.net" );
		CHECK( communique::impl::hostnameFromURL("blah.example-with-hyphen.net:443")=="blah.example-with-hyphen.net" );
		CHECK( communique::impl::hostnameFromURL("blah.example-with-hyphen.net")=="blah.example-with-hyphen.net" );
	}

}
*/
