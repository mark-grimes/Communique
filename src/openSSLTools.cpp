#include "communique/impl/openSSLTools.h"

#include <regex> // regular expression for hostname checking
#include <openssl/x509v3.h>


std::string communique::impl::convertASN1( ASN1_STRING *pOriginalString )
{
	unsigned char* pBuffer;
	int length=ASN1_STRING_to_UTF8( &pBuffer, pOriginalString );
	if( length<0 ) throw std::runtime_error( "Couldn't convert ASN1 string to UTF8" );
	std::string returnValue( reinterpret_cast<char*>(pBuffer), length );
	OPENSSL_free( pBuffer );
	return returnValue;
}

std::pair<std::string,std::string> communique::impl::convertNameEntry( X509_NAME_ENTRY* pEntry, bool shortName )
{
	ASN1_OBJECT *pObject = X509_NAME_ENTRY_get_object(pEntry);
	int myNID=OBJ_obj2nid(pObject);
	std::string objectName;
	if( shortName ) objectName=OBJ_nid2sn(myNID);
	else objectName=OBJ_nid2ln(myNID);

	std::string value=convertASN1( X509_NAME_ENTRY_get_data(pEntry) );

	return std::make_pair( objectName, value );
}

std::vector< std::pair<std::string,std::string> > communique::impl::convertName( X509_NAME* pName, bool shortName )
{
	std::vector< std::pair<std::string,std::string> > returnValue;

	for( size_t index=0; index<X509_NAME_entry_count(pName); ++index )
	{
		auto thisEntry=convertNameEntry( X509_NAME_get_entry(pName,index), shortName );
		returnValue.push_back(thisEntry);
	}
	return returnValue;
}

std::vector<std::string> communique::impl::getAlternateNames( X509* pCertificate )
{
	std::vector<std::string> returnValue;

	STACK_OF(GENERAL_NAME)* pAlternateNames=reinterpret_cast<STACK_OF(GENERAL_NAME)*>( X509_get_ext_d2i( pCertificate, NID_subject_alt_name, nullptr, nullptr ) );

	// Check each name within the extension
	for( int index=0; index<sk_GENERAL_NAME_num(pAlternateNames); ++index )
	{
		const GENERAL_NAME *pCurrentName=sk_GENERAL_NAME_value( pAlternateNames, index );

		if( pCurrentName->type == GEN_DNS )
		{
			std::string name=communique::impl::convertASN1( pCurrentName->d.dNSName );
			returnValue.push_back( name );
		}
	}

	return returnValue;
}

bool communique::impl::checkHostname( const std::string& hostname, const std::vector<std::string>& allowedList )
{
	// Loop over all allowed hosts checking for an exact match first. This will probably be
	// quicker than checking for wildcard matches as we go.
	for( const auto& allowedHost : allowedList )
	{
		if( hostname==allowedHost ) return true;
	}

	// Now loop again checking for wildcard matches
	const std::regex wildcardRegex("\\."); // Split by any occurrence of a dot

	// Split hostname by dots so that each bit between dots can be checked
	const std::sregex_iterator iHostBegin( hostname.begin(), hostname.end(), wildcardRegex );
	const std::sregex_iterator iEnd;

	for( const auto& allowedHost : allowedList )
	{
		std::sregex_iterator iAllowedMatch( allowedHost.begin(), allowedHost.end(), wildcardRegex );
		// Must be same number of dots
		if( std::distance(iAllowedMatch,iEnd) != std::distance(iHostBegin,iEnd) ) continue;

		// Loop over dot separated elements of both, and check for either an exact match or asterisk.
		std::sregex_iterator iHostMatch=iHostBegin;
		for( ; iHostMatch!=iEnd && iAllowedMatch!=iEnd; ++iHostMatch, ++iAllowedMatch )
		{
			// If any element breaks the rules, fail this allowedHost
			if( iAllowedMatch->prefix() != "*" && iAllowedMatch->prefix()!=iHostMatch->prefix() ) break;

			// Check to see if everything afterwards matches. For a lot of cases this acts
			// as an early cut out, but for the final loop it's actually required since
			// otherwise the bit after the last dot will never be checked.
			if( iAllowedMatch->suffix()==iHostMatch->suffix()  ) return true;
		}
	}

	// If control got this far then there were no matches
	return false;
}

std::string communique::impl::hostnameFromURL( const std::string& URL )
{
	const std::regex hostnameRegex("^([_[:alnum:]]+://)?([_.[:alnum:]\\-]+)(:|/)?");

	std::sregex_iterator iResult( URL.begin(), URL.end(), hostnameRegex );
	if( iResult->size()!=4 ) throw std::runtime_error( "communique::impl::hostnameFromURL - invalid regex match in '"+URL+"'");

	return (*iResult)[2];
}
