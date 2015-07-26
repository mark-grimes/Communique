#ifndef communique_impl_TLSHandler_h
#define communique_impl_TLSHandler_h

#include <functional>
#include <string>
#include <websocketpp/config/asio.hpp>

namespace communique
{

	namespace impl
	{
		/** @brief Methods common to Client and Server that deal with the TLS handshake.
		 *
		 * @author Mark Grimes
		 * @date 26/Jul/2015
		 */
		class TLSHandler
		{
		public:
			TLSHandler( websocketpp::config::asio::alog_type& logger );

			void setCertificateChainFile( const std::string& filename );
			void setPrivateKeyFile( const std::string& filename );
			void setVerifyFile( const std::string& filename );
			void setDiffieHellmanParamsFile( const std::string& filename );

			std::shared_ptr<boost::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl );
			bool verify_certificate( bool preverified, boost::asio::ssl::verify_context& context );
		protected:
			std::string certificateChainFileName_;
			std::string privateKeyFileName_;
			std::string verifyFileName_;
			std::string diffieHellmanParamsFileName_;
			websocketpp::config::asio::alog_type& logger_;
		};

	} // end of namespace impl
} // end of namespace communique

#endif // end of ifndef communique_impl_TLSHandler_h
