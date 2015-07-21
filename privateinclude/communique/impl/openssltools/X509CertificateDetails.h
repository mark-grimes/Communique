#ifndef communique_impl_openssltools_X509CertificateDetails_h
#define communique_impl_openssltools_X509CertificateDetails_h

#include <string>
#include <openssl/x509.h>

namespace communique
{

	namespace impl
	{
		namespace openssltools
		{
			/** @brief Class the queries the OpenSSL X509 certificate structure and gets information as std::strings.
			 * @author Mark Grimes
			 * @date 21/Jul/2015
			 */
			class X509CertificateDetails
			{
			public:
				X509CertificateDetails( X509* pCertificate );
				inline const std::string& subject() const;
				inline const std::string& issuer() const;
			protected:
				std::string subjectName_;
				std::string issuerName_;
			};

		} // end of namespace openssltools
	} // end of namespace impl
} // end of namespace communique

const std::string& communique::impl::openssltools::X509CertificateDetails::subject() const
{
	return subjectName_;
}

const std::string& communique::impl::openssltools::X509CertificateDetails::issuer() const
{
	return issuerName_;
}

#endif // end of ifndef communique_impl_openssltools_X509CertificateDetails_h
