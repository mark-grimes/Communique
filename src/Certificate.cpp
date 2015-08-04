#include "communique/impl/Certificate.h"
#include <stdio.h> // OpenSSL requires old school functions for file reading
#include <openssl/pem.h> // OpenSSL function to read the file from disk

#include "communique/impl/openSSLTools.h"


#include <openssl/x509v3.h>
#include <openssl/bn.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <sstream>
#include <iostream>


communique::impl::Certificate::Certificate( X509* pCertificate, bool takeOwnership ) : pOpenSSLHandle_(pCertificate)
{
	if( !takeOwnership ) pOpenSSLHandle_=X509_dup(pCertificate);
	else pOpenSSLHandle_=pCertificate;
	init();
}

communique::impl::Certificate::Certificate( const std::string& filename ) : pOpenSSLHandle_(nullptr)
{
	FILE* pFile=std::fopen(filename.c_str(),"r");
	if( !pFile ) throw std::runtime_error( "Unable to load the certificate file \""+filename+"\" from disk" );

	pOpenSSLHandle_=PEM_read_X509( pFile, nullptr, nullptr, nullptr );
	fclose(pFile);
	if( !pOpenSSLHandle_ ) throw std::runtime_error( "Unable to parse the certificate in file \""+filename+"\"" );

	init();
}

void communique::impl::Certificate::init()
{
	char* buffer;
	X509_NAME* pName;

	pName=X509_get_issuer_name(pOpenSSLHandle_);
	buffer=X509_NAME_oneline( pName, nullptr, 0 );
	issuerName_=buffer;
	OPENSSL_free(buffer);

	pName=X509_get_subject_name(pOpenSSLHandle_);
	auto subjectSplitDown=communique::impl::convertName(pName);
	for( const auto& entry : subjectSplitDown )
	{
		rdns_[entry.first].push_back(entry.second);
	}
	buffer=X509_NAME_oneline( pName, nullptr, 0 );
	subjectName_=buffer;
	OPENSSL_free(buffer);
}

communique::impl::Certificate::~Certificate()
{
	X509_free(pOpenSSLHandle_);
}

X509* communique::impl::Certificate::rawHandle()
{
	return pOpenSSLHandle_;
}

std::string communique::impl::Certificate::subject() const
{
	return subjectName_;
}

std::string communique::impl::Certificate::issuer() const
{
	return issuerName_;
}

std::vector<std::string> communique::impl::Certificate::subjectRDN( const std::string& RDN ) const
{
	const auto iFindResult=rdns_.find(RDN);
	if( iFindResult!=rdns_.end() ) return iFindResult->second;
	else return std::vector<std::string>();
}

std::string communique::impl::Certificate::subjectRDN( const std::string& RDN, size_t entry ) const
{
	const auto iFindResult=rdns_.find(RDN);
	if( iFindResult!=rdns_.end() ) return iFindResult->second.at(entry);
	else return std::string();
}

size_t communique::impl::Certificate::subjectRDNEntries( const std::string& RDN ) const
{
	const auto iFindResult=rdns_.find(RDN);
	if( iFindResult!=rdns_.end() ) return iFindResult->second.size();
	else return 0;
}

std::chrono::system_clock::time_point communique::impl::Certificate::validNotBefore() const
{
	return communique::impl::convertTime<std::chrono::system_clock>( X509_get_notBefore(pOpenSSLHandle_) );
}

std::chrono::system_clock::time_point communique::impl::Certificate::validNotAfter() const
{
	return communique::impl::convertTime<std::chrono::system_clock>( X509_get_notAfter(pOpenSSLHandle_) );
}

bool communique::impl::Certificate::dateIsValid() const
{
	auto notBefore=validNotBefore();
	auto notAfter=validNotAfter();
	auto now=std::chrono::system_clock::now();

	return (notBefore<now) && (now<notAfter);
}

bool communique::impl::Certificate::hostnameMatches( const std::string& hostname ) const
{
	// First check the common names in the subject, if that fails check all
	// the alternative names.
	if( checkHostname( hostname, subjectRDN("CN") ) ) return true;
	else return checkHostname( hostname, communique::impl::getAlternateNames(pOpenSSLHandle_) );
}
