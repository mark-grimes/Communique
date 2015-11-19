/** @file
 *
 * @brief Various free standing functions to help with using the OpenSSL interface.
 */
#ifndef communique_impl_openSSLTools_h
#define communique_impl_openSSLTools_h

#include <string>
#include <vector>
#include <stdexcept>
#include <ctime>

#include <openssl/asn1.h>
#include <openssl/x509.h>

namespace communique
{

	namespace impl
	{
		/** @brief Converts and ASN1_STRING to a std::string.
		 *
		 * @throws std::runtime_error if something went wrong.
		 *
		 * @author Mark Grimes
		 * @date 01/Aug/2015
		 */
		std::string convertASN1( ASN1_STRING *pOriginalString );

		/** @brief Converts a X509_NAME_ENTRY into separate std::string for its type and value.
		 *
		 * @parameter pEntry    The OpenSSL name entry to convert.
		 * @parameter shortName Whether or not the type returned should be the long or short name
		 *                      (e.g. "commonName" or "CN").
		 * @return    A pair of strings, where "first" is the type (e.g. "commonName") and "second"
		 *            is the value.
		 *
		 * @author Mark Grimes
		 * @date 01/Aug/2015
		 */
		std::pair<std::string,std::string> convertNameEntry( X509_NAME_ENTRY* pEntry, bool shortName=true );


		/** @brief Converts a X509_NAME_ENTRY into separate std::string for its type and value.
		 *
		 * Iterates over all of the X509_NAME_ENTRY in the X509_NAME and calls convertNameEntry on
		 * each one, putting all the results in a std::vector.
		 *
		 * @parameter pName    The OpenSSL name to convert.
		 * @parameter shortName Whether or not the type returned should be the long or short name
		 *                      (e.g. "commonName" or "CN").
		 * @return    A vector of string pairs. See the documentation for convertNameEntry for the
		 *            description of what each element is.
		 *
		 * @author Mark Grimes
		 * @date 01/Aug/2015
		 */
		std::vector< std::pair<std::string,std::string> > convertName( X509_NAME* pName, bool shortName=true );

		/** @brief Converts an ASN1_TIME to a std::chrono time.
		 *
		 * WARNING: Little or no consideration is (currently) taken for time zones. Depending on
		 * the time zone of the certificate and your system setting, the result could be out by as
		 * much as 24 hours (worst cast scenario, but unlikely).
		 *
		 * @author Mark Grimes
		 * @date 01/Aug/2015
		 */
		template<class T_clock>
		typename T_clock::time_point convertTime( ASN1_TIME* pTime );

		/** @brief Returns a list of all the alternative hostnames in a certificate.
		 *
		 * Only includes DNS names in the Subject Alternative Name (SAN) list.
		 *
		 * @author Mark Grimes
		 * @date 03/Aug/2015
		 */
		std::vector<std::string> getAlternateNames( X509* pCertificate );

		/** @brief Checks whether the hostname matches any of the names in the list.
		 *
		 * Takes account of exact matches and wildcard matches, e.g. will return true
		 * for a hostname of "blah.google.com" is requested against a list containing
		 * "*.google.com".
		 *
		 * Note that, in line with the standard, "google.com" will not match against
		 * "*.google.com".
		 *
		 * @parameter hostname     The hostname that is being checked. Should be the exact name,
		 *                         i.e. not containing wildcards.
		 * @parameter allowedList  A list of all allowed names and/or wildcard entries.
		 * @return    Returns true if the hostname matches any of the entries in the list.
		 *
		 * @author Mark Grimes
		 * @date 03/Aug/2015
		 */
		bool checkHostname( const std::string& hostname, const std::vector<std::string>& allowedList );
	} // end of namespace impl
} // end of namespace communique


//
// Implementation of templated methods required in the header file
//
template<class T_clock>
typename T_clock::time_point communique::impl::convertTime( ASN1_TIME* pTime )
{
	std::string timeAsString=convertASN1(pTime);
	size_t digitChars=0; // The number of characters at the start of the string that are digits
	for( digitChars=0; digitChars<timeAsString.size(); ++digitChars )
	{
		// Loop until the ASCII code is not a digit
		if( timeAsString[digitChars]<'0' || timeAsString[digitChars]>'9' ) break;
	}
	// There can be 12 digits, in which case year is 2 digits, or 14 when the year is 4 digits. Any
	// characters after that (I think) describe the time zone, which I don't fully know how to interpret.
	if( digitChars!=12 && digitChars!=14 ) throw std::runtime_error( "Cannot convert ASN1 time '"+timeAsString+"', incorrect number of digits" );

	size_t position=0; // Current position in string
	std::tm time; // Not that this structure has years since 1900, and January is month 0.
	time.tm_isdst=-1; // No information about Daylight Saving Time

	if( digitChars==12 ) // Year is two digits
	{
		time.tm_year=std::stoi( timeAsString.substr(position,2) );
		position+=2;
		// I want the number of years since 1900. I'm going to assume the
		// year is 20xx, and never 19xx. Who issues certificates before 2000?
		time.tm_year+=100; // assume chopped off bit was "20" not "19"
	}
	else // Year is 4 digits
	{
		time.tm_year=std::stoi( timeAsString.substr(position,4) );
		position+=4;
		time.tm_year-=1900; // I want the number of years since 1900
	}

	time.tm_mon=std::stoi( timeAsString.substr(position,2) )-1; // "-1" because month should start at zero
	position+=2;

	time.tm_mday=std::stoi( timeAsString.substr(position,2) );
	position+=2;

	time.tm_hour=std::stoi( timeAsString.substr(position,2) );
	position+=2;

	time.tm_min=std::stoi( timeAsString.substr(position,2) );
	position+=2;

	time.tm_sec=std::stoi( timeAsString.substr(position,2) );
	position+=2;

	std::string timeZone=timeAsString.substr(position);
	// Todo - figure out what to do with the time zone information

	return T_clock::from_time_t( mktime(&time) );
}

#endif // end of ifndef communique_impl_openSSLTools_h
