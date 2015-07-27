#include "communique/impl/Certificate.h"
#include <regex>
#include <stdio.h> // OpenSSL requires old school functions for file reading
#include <openssl/pem.h> // OpenSSL function to read the file from disk

// TODO - Remove once fully tested
#include <iostream>

communique::impl::Certificate::Certificate( X509* pCertificate, bool takeOwnership ) : pOpenSSLHandle_(pCertificate), handleIsOwned_(takeOwnership)
{
	init();
}

communique::impl::Certificate::Certificate( const std::string& filename ) : pOpenSSLHandle_(nullptr), handleIsOwned_(true)
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

	buffer=X509_NAME_oneline( X509_get_issuer_name(pOpenSSLHandle_), nullptr, 0 );
	issuerName_=buffer;
	OPENSSL_free(buffer);

	buffer=X509_NAME_oneline( X509_get_subject_name(pOpenSSLHandle_), nullptr, 0 );
	subjectName_=buffer;
	OPENSSL_free(buffer);

	// Scan the subject and try to split it into the Relative Distinguished Names
	const std::regex word_regex("/([A-Z]+)=");
	auto iMatch=std::sregex_iterator( subjectName_.begin(), subjectName_.end(), word_regex );

	std::string lastRDN;
	std::string lastValue;
	for( ; iMatch!=std::sregex_iterator(); ++iMatch )
	{
		if( !lastRDN.empty() ) rdns_[lastRDN]=iMatch->prefix();
		lastRDN=(*iMatch)[1];
		lastValue=iMatch->suffix(); // Need to record this in case it's the last loop
	}
	// Also need to add the one for the final match
	if( !lastRDN.empty() ) rdns_[lastRDN]=lastValue;

}

communique::impl::Certificate::~Certificate()
{
	if(handleIsOwned_) X509_free(pOpenSSLHandle_);
}

X509* communique::impl::Certificate::rawHandle()
{
	return pOpenSSLHandle_;
}

std::string communique::impl::Certificate::subject() const
{
	return subjectName_;
}

std::string communique::impl::Certificate::subjectRDN( const std::string& RDN ) const
{
	const auto iFindResult=rdns_.find(RDN);
	if( iFindResult!=rdns_.end() ) return iFindResult->second;
	else return ""; // or throw exception???
}

std::string communique::impl::Certificate::issuer() const
{
	return issuerName_;
}
