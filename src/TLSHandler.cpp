#include "communique/impl/TLSHandler.h"

#include "communique/impl/Certificate.h"

communique::impl::TLSHandler::TLSHandler( websocketpp::config::asio::alog_type& logger )
	: logger_(logger)
{
	// No operation besides the initialiser list
}

void communique::impl::TLSHandler::setCertificateChainFile( const std::string& filename )
{
	certificateChainFileName_=filename;
}

void communique::impl::TLSHandler::setPrivateKeyFile( const std::string& filename )
{
	privateKeyFileName_=filename;
}

void communique::impl::TLSHandler::setVerifyFile( const std::string& filename )
{
	verifyFileName_=filename;
}

void communique::impl::TLSHandler::setDiffieHellmanParamsFile( const std::string& filename )
{
	diffieHellmanParamsFileName_=filename;
}

websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> communique::impl::TLSHandler::on_tls_init( websocketpp::connection_hdl hdl ) const
{
	namespace asio=websocketpp::lib::asio;
	websocketpp::lib::shared_ptr<asio::ssl::context> pContext( new asio::ssl::context(asio::ssl::context::tlsv1) );
	pContext->set_options( asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use );
	//pContext->set_password_callback( websocketpp::lib::bind( &server::get_password, this ) );
	if( !certificateChainFileName_.empty() ) pContext->use_certificate_chain_file( certificateChainFileName_ );
	if( !privateKeyFileName_.empty() ) pContext->use_private_key_file( privateKeyFileName_, asio::ssl::context::pem );
	if( !verifyFileName_.empty() )
	{
		pContext->set_verify_mode( asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert );
		pContext->load_verify_file( verifyFileName_ );
		pContext->set_verify_callback( std::bind( &TLSHandler::verify_certificate, this, std::placeholders::_1, std::placeholders::_2 ) );
	}
	else pContext->set_verify_mode( asio::ssl::verify_none );
	if( !diffieHellmanParamsFileName_.empty() ) pContext->use_tmp_dh_file( diffieHellmanParamsFileName_ );

	return pContext;
}

bool communique::impl::TLSHandler::verify_certificate( bool preverified, websocketpp::lib::asio::ssl::verify_context& context ) const
{
	constexpr int maxDepth=8;
	constexpr bool verbose=true;

	X509_STORE_CTX *pRawContext=context.native_handle();

	int errorCode=X509_STORE_CTX_get_error( pRawContext );
	int depth=X509_STORE_CTX_get_error_depth( pRawContext );

	X509* pCurrentCertificate=X509_STORE_CTX_get_current_cert( pRawContext );
	communique::impl::Certificate certificateDetails(pCurrentCertificate);

	/*
	 * Catch a too long certificate chain. The depth limit set using
	 * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
	 * that whenever the "depth>verify_depth" condition is met, we
	 * have violated the limit and want to log this error condition.
	 * We must do it here, because the CHAIN_TOO_LONG error would not
	 * be found explicitly; only errors introduced by cutting off the
	 * additional certificates would be logged.
	 */
	if( depth > maxDepth )
	{
		preverified=0;
		errorCode=X509_V_ERR_CERT_CHAIN_TOO_LONG;
		X509_STORE_CTX_set_error( pRawContext, errorCode );
	}

	if( !preverified )
	{
		std::string errorMessage="X509 verification error:"+std::to_string(errorCode)+":"+X509_verify_cert_error_string(errorCode)+":depth="+std::to_string(depth)+":subject="+certificateDetails.subject();
		if( errorCode==X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT ) errorMessage+=":issuer="+certificateDetails.issuer();
		logger_.write( websocketpp::log::alevel::debug_handshake, errorMessage );
	}
	else if( verbose )
	{
		logger_.write( websocketpp::log::alevel::debug_handshake, "X509 verification depth="+std::to_string(depth)+":"+certificateDetails.subject() );
	}

	return preverified;
}
