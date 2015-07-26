#ifndef communique_Client_h
#define communique_Client_h

#include <memory>
#include <communique/IConnection.h>

namespace communique
{

	/** @brief Class that handles communication to a remote server
	 *
	 * @author Mark Grimes (kknb1056@gmail.com)
	 * @date 30/Sep/2014
	 */
	class Client : public communique::IConnection
	{
	public:
		Client();
		Client( Client&& otherClient ) noexcept;
		~Client();

		/** @brief Attempts to connect to the URI provided. Returns before the connection is established. */
		void connect( const std::string& URI );

		/** @brief Returns true if the connection is established. If the handshake is still ongoing, blocks until that is finished. */
		bool isConnected();
		/** @brief Returns true if the connection is closed. If in the process of disconnecting, blocks until that is finished.
		 *
		 * Note that this is subtly different from "!isConnected()". Since the blocking conditions are different the two
		 * functions are useful to stop different race conditions. This function only blocks while closing, isConnected()
		 * only blocks while opening.
		 */
		bool isDisconnected();
		void disconnect();
		void setCertificateChainFile( const std::string& filename );
		void setPrivateKeyFile( const std::string& filename );
		void setVerifyFile( const std::string& filename );

		virtual void sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler ) override;
		virtual void sendInfo( const std::string& message ) override;
		virtual void setInfoHandler( std::function<void(const std::string&)> infoHandler ) override;
		virtual void setInfoHandler( std::function<void(const std::string&,communique::IConnection*)> infoHandler ) override;
		virtual void setRequestHandler( std::function<std::string(const std::string&)> requestHandler ) override;
		virtual void setRequestHandler( std::function<std::string(const std::string&,communique::IConnection*)> requestHandler ) override;

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
		std::unique_ptr<class ClientPrivateMembers> pImple_;
	};

} // end of namespace communique

#endif // end of ifndef communique_Client_h
