#ifndef communique_IConnection_h
#define communique_IConnection_h

#include <functional>
#include <memory>

namespace communique
{

	/** @brief Abstract interface to connections between a client and a server.
	 *
	 * @author Mark Grimes (kknb1056@gmail.com)
	 * @date 30/Sep/2014
	 */
	class IConnection
	{
	public:
		virtual ~IConnection() {}

		/** @brief Send a request that requires a response
		 * @parameter message          The information to send. This needn't be ASCII, std::string is just
		 *                             used as a convenient container.
		 * @parameter responseHandler A function to receive the response. The signature has to be
		 *                             "void responseHandler( void* pResponse, size_t size )"
		 */
		virtual void sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler ) = 0;

		/** @brief Send information that does not require a response
		 * @parameter message          The information to send. This needn't be ASCII, std::string is just
		 *                             used as a convenient container.
		 * @parameter length           The size of the information.
		 */
		virtual void sendInfo( const std::string& message ) = 0;

		/** @brief Sets the function that will be notified when information comes in (no response required) */
		virtual void setInfoHandler( std::function<void(const std::string&)> infoHandler ) = 0;
		virtual void setInfoHandler( std::function<void(const std::string&,std::weak_ptr<communique::IConnection>)> infoHandler ) = 0;

		/** @brief Sets the function that will be notified when requests come in (response required) */
		virtual void setRequestHandler( std::function<std::string(const std::string&)> requestHandler ) = 0;
		virtual void setRequestHandler( std::function<std::string(const std::string&,std::weak_ptr<communique::IConnection>)> requestHandler ) = 0;
	};

} // end of namespace communique

#endif // end of ifndef communique_IConnection_h
