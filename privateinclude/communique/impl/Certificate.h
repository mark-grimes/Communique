#ifndef communique_impl_Certificate_h
#define communique_impl_Certificate_h

#include "communique/ICertificate.h"
#include <map>
#include <openssl/x509.h>

namespace communique
{

	namespace impl
	{
		/** @brief Implementation of the certificate class which encapsulates X509 certificate information.
		 *
		 * @author Mark Grimes
		 * @date 26/Jul/2015
		 */
		class Certificate : public communique::ICertificate
		{
		public:
			Certificate( X509* pCertificate, bool takeOwnership=false );
			Certificate( const std::string& filename );
			virtual ~Certificate();

			X509* rawHandle();

			virtual std::string subject() const override;
			virtual std::string subjectRDN( const std::string& RDN ) const override;

			virtual std::string issuer() const override;

		protected:
			X509* pOpenSSLHandle_;
			bool handleIsOwned_; ///< True if this instance owns pOpenSSLHandle_
			std::string subjectName_;
			std::map<std::string,std::string> rdns_; // map of Relative Distinguished Names
			std::string issuerName_;
			void init();
		};

	} // end of namespace impl
} // end of namespace communique

#endif // end of ifndef communique_impl_Certificate_h
