#include <communique/impl/Certificate.h>
#include "../catch.hpp"

#include "../testinputs.h" // Constants like locations of the test inputs


SCENARIO( "Test that x509 certificates can be loaded and details retrieved properly", "[local][tools][balls]" )
{
	GIVEN( "A certificate loaded from the test directory" )
	{
		std::unique_ptr<communique::impl::Certificate> pTestCertificate; // Need to use a pointer because I want the constructor in a test condition
		REQUIRE_NOTHROW( pTestCertificate.reset( new communique::impl::Certificate( testinputs::testFileDirectory+"server_cert.pem" ) ) );

		WHEN( "I check the subject, issuer, etcetera" )
		{
			CHECK( pTestCertificate->issuer()=="/C=AU/ST=Some-State/O=Test Certificate Authority/CN=Test Certificate Authority" );
			CHECK( pTestCertificate->subject()=="/C=AU/ST=Some-State/O=Test Server/CN=www.testserver.com" );
			CHECK( pTestCertificate->subjectRDN("C")=="AU" );
			CHECK( pTestCertificate->subjectRDN("ST")=="Some-State" );
			CHECK( pTestCertificate->subjectRDN("O")=="Test Server" );
			CHECK( pTestCertificate->subjectRDN("CN")=="www.testserver.com" );
		}
	}
}
