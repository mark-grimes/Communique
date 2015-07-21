#include "communique/impl/openssltools/X509CertificateDetails.h"

communique::impl::openssltools::X509CertificateDetails::X509CertificateDetails( X509* pCertificate )
{
	char* buffer;

	buffer=X509_NAME_oneline( X509_get_subject_name(pCertificate), nullptr, 0 );
	subjectName_=buffer;
	OPENSSL_free(buffer);

	buffer=X509_NAME_oneline( X509_get_issuer_name(pCertificate), nullptr, 0 );
	issuerName_=buffer;
	OPENSSL_free(buffer);
}
