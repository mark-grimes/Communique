#include "communique/impl/openSSLTools.h"
#include <openssl/x509v3.h>

//
// Unnamed namespace for things only used in this file
//
namespace
{
	/** Splits a string into individual parts delimited by a user defined delimiter(s).
	 *
	 * The user can specify more than one character in the delimiter, in which case any of
	 * them will be used, rather than the whole string.
	 *
	 * Note that this functions differently to splitByWhitespace in that any single delimiter
	 * will produce a new element - multiple contiguous delimiters will produce multiple empty
	 * elements in the return value. In splitByWhitespace contiguous whitespace is considered
	 * as a single delimiter.
	 *
	 * @param[in] stringToSplit    The string to split into elements.
	 * @param[in] delimiters       The delimiters to use. If more than one character is specified
	 *                             then any single one is considered a delimiter.
	 * @return                     A vector where each element is a portion of the string.
	 * @author Mark Grimes
	 * @date 16/Jul/2013
	 */
	std::vector<std::string> splitByDelimiters( const std::string& stringToSplit, const std::string& delimiters )
	{
		std::vector<std::string> returnValue;

		size_t currentPosition=0;
		size_t nextDelimiter=0;
		do
		{
			// Find the next occurence of one of the delimiters and subtract everything up to that point
			nextDelimiter=stringToSplit.find_first_of( delimiters, currentPosition );
			std::string element=stringToSplit.substr( currentPosition, nextDelimiter-currentPosition );
			returnValue.push_back(element);

			// Move on to the next part of the string
			if( nextDelimiter+1<stringToSplit.size() ) currentPosition=nextDelimiter+1;
			else nextDelimiter=std::string::npos;

		} while( nextDelimiter!=std::string::npos );

		return returnValue;
	}
} // end of the unnamed namespace

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

	// Split hostname by dots so that each bit between dots can be checked
	auto hostnameSplitByDots=::splitByDelimiters( hostname, "." );

	for( const auto& allowedHost : allowedList )
	{
		auto allowedHostSplitByDots=::splitByDelimiters( allowedHost, "." );
		// Must be same number of dots
		if( hostnameSplitByDots.size()!=allowedHostSplitByDots.size() ) continue;

		// Loop over dot separated elements of both, and check for either an exact match or asterisk.
		bool allElementsMatch=true;
		for( auto iHost=hostnameSplitByDots.begin(),iAllowed=allowedHostSplitByDots.begin();
			iHost!=hostnameSplitByDots.end() && iAllowed!=allowedHostSplitByDots.end();
			++iHost, ++iAllowed )
		{
			// If any element breaks the rules, fail this allowedHost
			if( *iAllowed != "*" && *iAllowed != *iHost )
			{
				allElementsMatch=false;
				break;
			}
		}
		if( allElementsMatch ) return true;
	}

	// If control got this far then there were no matches
	return false;
}

std::string communique::impl::hostnameFromURL( const std::string& URL )
{
	//
	// Regular expression is not available in gcc less than 5 (headers are there earlier
	// but with no implementation, so you get link errors). So I need to disable this solution
	// and do it by hand.
	//
	/*
	const std::regex hostnameRegex("^([_[:alnum:]]+://)?([_.[:alnum:]\\-]+)(:|/)?");

	std::sregex_iterator iResult( URL.begin(), URL.end(), hostnameRegex );
	if( iResult->size()!=4 ) throw std::runtime_error( "communique::impl::hostnameFromURL - invalid regex match in '"+URL+"'");

	return (*iResult)[2];
	*/
	std::string hostname;
	auto isUppercase=[](const char character){ return (character>='A' && character<='Z'); };
	auto isLowercase=[](const char character){ return (character>='a' && character<='z'); };
	auto isNumeric=[](const char character){ return (character>='0' && character<='9'); };
	auto isAlphaNumeric=[&](const char character){ return isUppercase(character) || isLowercase(character) || isNumeric(character); };
	auto isProtocol=[&](const char character){ return isAlphaNumeric(character) || character=='_'; };
	auto isHostname=[&](const char character){ return isProtocol(character) || character=='.' || character=='-'; };
	bool alreadyParsedProtocol=false;
	bool alreadyInvalidProtocol=false;

	for( std::string::const_iterator iChar=URL.begin(); iChar!=URL.end(); ++iChar )
	{
		if( !alreadyParsedProtocol )
		{
			if( isProtocol(*iChar) ) hostname+=*iChar; // Could be part of the hostname if there is no protocol
			else if( isHostname(*iChar) )
			{
				hostname+=*iChar;
				alreadyParsedProtocol=true; // Can't be part of the protocol anymore
			}
			else if( *iChar==':' )
			{
				// See if this is the start of "://"
				if( std::distance(iChar,URL.end())>=3 )
				{
					if( *(++iChar)=='/' && *(++iChar)=='/' )
					{
						// This is the protocol separator, so clear what's been parsed so far
						hostname.clear();
						alreadyParsedProtocol=true;
					}
					else return hostname; // URL didn't have a valid protocol so this must be start of port
				}
				else return hostname; // URL didn't have enough characters for "://" so this must be end start of port number
			}
			else if( *iChar=='/' ) return hostname; // URL didn't have a valid protocol
			else throw std::runtime_error( "hostnameFromURL called with string that has invalid characters" );
		}
		else
		{
			if( isHostname(*iChar) ) hostname+=*iChar;
			else if( *iChar==':' ) return hostname; // must be start of port specifier
			else if( *iChar=='/' ) return hostname; // URL didn't have a valid protocol
			else throw std::runtime_error( "hostnameFromURL called with string that has invalid characters" );
		}
	}

	return hostname;
}
