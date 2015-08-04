#include <communique/impl/Certificate.h>
#include "../catch.hpp"

#include "../testinputs.h" // Constants like locations of the test inputs


SCENARIO( "Test that x509 certificates can be loaded and details retrieved properly", "[local][tools][certificate]" )
{
	GIVEN( "A certificate loaded from the test directory" )
	{
		std::unique_ptr<communique::impl::Certificate> pTestCertificate; // Need to use a pointer because I want the constructor in a test condition
		REQUIRE_NOTHROW( pTestCertificate.reset( new communique::impl::Certificate( testinputs::testFileDirectory+"server_cert.pem" ) ) );

		WHEN( "I check the subject, issuer, etcetera" )
		{
			CHECK( pTestCertificate->issuer()=="/C=AU/ST=Some-State/O=Test Certificate Authority/CN=Test Certificate Authority" );
			CHECK( pTestCertificate->subject()=="/C=AU/ST=Some-State/O=Test Server/CN=www.testserver.com" );
			CHECK( pTestCertificate->subjectRDNEntries("C")==1 );
			CHECK( pTestCertificate->subjectRDN("C",0)=="AU" );
			CHECK( pTestCertificate->subjectRDNEntries("ST")==1 );
			CHECK( pTestCertificate->subjectRDN("ST",0)=="Some-State" );
			CHECK( pTestCertificate->subjectRDNEntries("O")==1 );
			CHECK( pTestCertificate->subjectRDN("O",0)=="Test Server" );
			CHECK( pTestCertificate->subjectRDNEntries("CN")==1 );
			CHECK( pTestCertificate->subjectRDN("CN",0)=="www.testserver.com" );

			// I guess at some point these tests are going to stop working
			CHECK( pTestCertificate->dateIsValid() );
		}
	}

	GIVEN( "Google.co.uk's certificate" )
	{
		std::unique_ptr<communique::ICertificate> pTestCertificate; // Need to use a pointer because I want the constructor in a test condition
		REQUIRE_NOTHROW( pTestCertificate.reset( new communique::impl::Certificate( testinputs::testFileDirectory+"google_co_uk.cert.pem" ) ) );

		WHEN( "I check the subject, issuer, etcetera" )
		{
			std::string testString;
			CHECK( pTestCertificate->issuer()=="/C=US/O=Google Inc/CN=Google Internet Authority G2" );
			CHECK( pTestCertificate->subject()=="/C=US/ST=California/L=Mountain View/O=Google Inc/CN=google.com" );

			CHECK( pTestCertificate->subjectRDNEntries("C")==1 );
			REQUIRE_NOTHROW( testString=pTestCertificate->subjectRDN("C",0) );
			CHECK( testString=="US" );

			CHECK( pTestCertificate->subjectRDNEntries("ST")==1 );
			REQUIRE_NOTHROW( testString=pTestCertificate->subjectRDN("ST",0) );
			CHECK( testString=="California" );

			CHECK( pTestCertificate->subjectRDNEntries("L")==1 );
			REQUIRE_NOTHROW( testString=pTestCertificate->subjectRDN("L",0) );
			CHECK( testString=="Mountain View" );

			CHECK( pTestCertificate->subjectRDNEntries("O")==1 );
			REQUIRE_NOTHROW( testString=pTestCertificate->subjectRDN("O",0) );
			CHECK( testString=="Google Inc" );

			CHECK( pTestCertificate->subjectRDNEntries("CN")==1 );
			REQUIRE_NOTHROW( testString=pTestCertificate->subjectRDN("CN",0) );
			CHECK( testString=="google.com" );

			// I guess at some point these tests are going to stop working
			CHECK( pTestCertificate->dateIsValid() );
		}

		WHEN( "I check hostname matches in a wildcard certificate" )
		{
			// This google certificate has loads of alternate names and wildcard
			// matches, which makes it great for testing.
			CHECK( pTestCertificate->hostnameMatches("google.com")==true );
			// Some of the alternate names
			CHECK( pTestCertificate->hostnameMatches("goo.gl")==true );
			CHECK( pTestCertificate->hostnameMatches("doubleclick.net")==true );
			CHECK( pTestCertificate->hostnameMatches("android.com")==true );
			// Try some wildcard matches
			CHECK( pTestCertificate->hostnameMatches("mail.google.com")==true );
			CHECK( pTestCertificate->hostnameMatches("blahblahblah.google.com")==true );
			CHECK( pTestCertificate->hostnameMatches("blah.metric.gstatic.com")==true );
			CHECK( pTestCertificate->hostnameMatches("metric.gstatic.com")==true ); // certificate also has "*.gstatic.com"
			// Make sure some of the bare hostnames don't match against the wildcard names
			CHECK( pTestCertificate->hostnameMatches("2mdn.net")==false );

			// Try some other random stuff that should fail
			CHECK( pTestCertificate->hostnameMatches("www.microsoft.com")==false );
			CHECK( pTestCertificate->hostnameMatches("www.apple.com")==false );
			CHECK( pTestCertificate->hostnameMatches("apple.com")==false );
			CHECK( pTestCertificate->hostnameMatches("asdlkfj*asdflj")==false );
		}
	}
}
