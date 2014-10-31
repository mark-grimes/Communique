#ifndef comm_Server_h
#define comm_Server_h

#include <memory>
#include <functional>
#include <vector>

//
// Forward declarations
//
namespace comm
{
	class IConnection;
}

namespace comm
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

		bool listen( size_t port );
		void stop();

		void setDefaultInfoHandler( std::function<void(const std::string&)> infoHandler );
		void setDefaultInfoHandler( std::function<void(const std::string&,comm::IConnection*)> infoHandler );
		void setDefaultRequestHandler( std::function<const std::string(const std::string&)> requestHandler );
		void setDefaultRequestHandler( std::function<const std::string(const std::string&,comm::IConnection*)> requestHandler );

		std::vector<std::shared_ptr<comm::IConnection> > currentConnections();
	private:
		/// Pimple idiom to hide the transport details
		std::unique_ptr<class ServerPrivateMembers> pImple_;
	};

} // end of namespace comm

#endif // end of ifndef comm_Server_h
