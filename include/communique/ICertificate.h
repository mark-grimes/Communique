#ifndef communique_ICertificate_h
#define communique_ICertificate_h

#include <string>
#include <vector>
#include <chrono>

namespace communique
{

	/** @brief Interface for a certificate class which encapsulates X509 certificate information.
	 *
	 * @author Mark Grimes
	 * @date 26/Jul/2015
	 */
	class ICertificate
	{
	public:
		virtual ~ICertificate() {}

		virtual std::string subject() const = 0;
		virtual std::vector<std::string> subjectRDN( const std::string& RDN ) const = 0;
		virtual std::string subjectRDN( const std::string& RDN, size_t entry ) const = 0;
		virtual size_t subjectRDNEntries( const std::string& RDN ) const = 0;

		virtual std::string issuer() const = 0;
		virtual std::chrono::system_clock::time_point validNotBefore() const = 0;
		virtual std::chrono::system_clock::time_point validNotAfter() const = 0;
		virtual bool dateIsValid() const = 0;
		virtual bool hostnameMatches( const std::string& hostname ) const = 0;
	};

} // end of namespace communique

#endif // end of ifndef communique_ICertificate_h
