#ifndef communique_Server_h
#define communique_Server_h

#include <memory>
#include <functional>
#include <vector>
#include <system_error>

//
// Forward declarations
//
namespace communique
{
	class IConnection;
}

namespace communique
{

	/** @brief Class that handles communication from the local objects to the outside world.
	 *
	 * Sets up the connection to listen on a port, and creates connection wrappers when a new
	 * connection is made.
	 *
	 * @author Mark Grimes (kknb1056@gmail.com)
	 * @date 27/Sep/2014
	 */
	class Server
	{
	public:
		Server();
		~Server();

		bool listen( size_t port, std::error_code& error );
		bool listen( size_t port );
		bool listen( const std::string& address, size_t port, std::error_code& error );
		bool listen( const std::string& address, size_t port );
		void stop();
		void setCertificateChainFile( const std::string& filename );
		void setPrivateKeyFile( const std::string& filename );
		void setVerifyFile( const std::string& filename );
		void setDiffieHellmanParamsFile( const std::string& filename );

		void setDefaultInfoHandler( std::function<void(const std::string&)> infoHandler );
		void setDefaultInfoHandler( std::function<void(const std::string&,std::weak_ptr<communique::IConnection>)> infoHandler );
		void setDefaultRequestHandler( std::function<std::string(const std::string&)> requestHandler );
		void setDefaultRequestHandler( std::function<std::string(const std::string&,std::weak_ptr<communique::IConnection>)> requestHandler );

		enum class HTTPBehaviour { ignore, acknowledge, file_serve };
		void setHTTPBehaviour( HTTPBehaviour behaviour );
		/** @brief If HTTP behaviour is file_serve, this sets the root directory to serve files from.
		 *
		 * Calling this automatically switches the HTTP behaviour to file_serve. You can set the directory to
		 * a relative path although you are strongly encouraged to set absolute paths. Otherwise you may inadvertantly
		 * start exposing private files if you change the working directory. */
		void setFileServeRoot( const std::string& rootDirectory );

		std::vector<std::weak_ptr<communique::IConnection> > currentConnections();

		/** @brief Set where error messages are sent */
		void setErrorLogLocation( std::ostream& outputStream );
		/** @brief Set the verbosity of error messages. Implementation specific, but zero is none 0xffffffff is everything. */
		void setErrorLogLevel( uint32_t level );
		/** @brief Set where log message about access requests are sent */
		void setAccessLogLocation( std::ostream& outputStream );
		/** @brief Set the verbosity of the access log. Implementation specific, but zero is none 0xffffffff is everything. */
		void setAccessLogLevel( uint32_t level );
	private:
		/// Pimple idiom to hide the transport details
		std::unique_ptr<class ServerPrivateMembers> pImple_;
	};

} // end of namespace communique

#endif // end of ifndef communique_Server_h
