#include <communique/impl/openssltools/X509CertificateDetails.h>
#include "../catch.hpp"
#include <stdio.h> // OpenSSL requires old school functions
#include <openssl/pem.h> // OpenSSL function to read the file from disk

#include "../testinputs.h" // Constants like locations of the test inputs


// Use the unnamed namespace for things only in this file
namespace
{
	/** Loads a certificate from disk using OpenSSL and automatically frees it when it goes out of scope.*/
	class Certificate
	{
	public:
		Certificate( const std::string& filename ) : pOpenSSLHandle_(nullptr)
		{
			FILE* pFile=std::fopen(filename.c_str(),"r");
			if( !pFile ) throw std::runtime_error( "Unable to load the certificate file \""+filename+"\" from disk" );

			pOpenSSLHandle_=PEM_read_X509( pFile, nullptr, nullptr, nullptr );
			fclose(pFile);
			if( !pOpenSSLHandle_ ) throw std::runtime_error( "Unable to parse the certificate in file \""+filename+"\"" );
		}
		~Certificate()
		{
			X509_free(pOpenSSLHandle_);
		}
		X509* rawHandle()
		{
			return pOpenSSLHandle_;
		}
	private:
		X509* pOpenSSLHandle_;
	};
}

SCENARIO( "Test that x509 certificate details can be retrieved properly", "[local][tools]" )
{
	GIVEN( "A certificate loaded from the test directory" )
	{
		std::unique_ptr< ::Certificate > pTestCertificate; // Need to use a pointer because I want the constructor in a test condition
		REQUIRE_NOTHROW( pTestCertificate.reset( new ::Certificate( testinputs::testFileDirectory+"server_cert.pem" ) ) );

		WHEN( "I check the subject, issuer, etcetera" )
		{
			communique::impl::openssltools::X509CertificateDetails certificateDetails( pTestCertificate->rawHandle() );

			CHECK( certificateDetails.subject()=="/C=AU/ST=Some-State/O=Test Server/CN=www.testserver.com" );
			CHECK( certificateDetails.issuer()=="/C=AU/ST=Some-State/O=Test Certificate Authority/CN=Test Certificate Authority" );
		}
	}
}
