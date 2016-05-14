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
			TLSHandler( websocketpp::config::asio::alog_type& logger, bool isServer );

			void setCertificateChainFile( const std::string& filename );
			void setPrivateKeyFile( const std::string& filename );
			void setVerifyFile( const std::string& filename );
			void setDiffieHellmanParamsFile( const std::string& filename );

			std::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl ) const;
			bool verify_certificate( bool preverified, websocketpp::lib::asio::ssl::verify_context& context ) const;
		protected:
			bool isServer_;

			/** @brief Whether or not to allow anonymous authentication.
			 *
			 * This is currently always true but will be configurable in future. Ignored if the user sets
			 * verification details with setVerifyFile. */
			bool allowAnonAuthentication_;
			std::string certificateChainFileName_;
			std::string privateKeyFileName_;
			std::string verifyFileName_;
			std::string diffieHellmanParamsFileName_;
			websocketpp::config::asio::alog_type& logger_;

			// Diffie Hellman parameters if the user doesn't set any. These are generated at
			// compile time and put in TLSHandler_fallbackDiffieHellmanParameters_.cpp. (it's
			// not defined in TLSHandler.cpp because it's auto generated).
			static const std::string fallbackDiffieHellmanParameters_;
		};

	} // end of namespace impl
} // end of namespace communique

#endif // end of ifndef communique_impl_TLSHandler_h
