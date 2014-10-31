#ifndef comm_Client_h
#define comm_Client_h

#include <memory>
#include <comm/IConnection.h>

namespace comm
{

	/** @brief Class that handles communication to a remote server
	 *
	 * @author Mark Grimes (kknb1056@gmail.com)
	 * @date 30/Sep/2014
	 */
	class Client : public comm::IConnection
	{
	public:
		Client();
		~Client();

		void connect( const std::string& URI );
		void disconnect();
		virtual void sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler ) override;
		virtual void sendInfo( const std::string& message ) override;
		virtual void setInfoHandler( std::function<void(const std::string&)> infoHandler ) override;
		virtual void setInfoHandler( std::function<void(const std::string&,comm::IConnection*)> infoHandler ) override;
		virtual void setRequestHandler( std::function<std::string(const std::string&)> requestHandler ) override;
		virtual void setRequestHandler( std::function<std::string(const std::string&,comm::IConnection*)> requestHandler ) override;
	private:
		/// Pimple idiom to hide the transport details
		std::unique_ptr<class ClientPrivateMembers> pImple_;
	};

} // end of namespace comm

#endif // end of ifndef comm_Client_h
