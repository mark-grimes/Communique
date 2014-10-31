#ifndef comm_impl_Connection_h
#define comm_impl_Connection_h

#include <comm/IConnection.h>
#include <functional>
#include <list>

#define _WEBSOCKETPP_CPP11_STL_ // Make sure websocketpp uses c++11 features in preference to boost ones
#include <websocketpp/connection.hpp>
#include <websocketpp/config/asio.hpp>

#include "comm/impl/Message.h"
#include "comm/impl/UniqueTokenStorage.h"

namespace comm
{

	namespace impl
	{
		/** @brief Implementation of one end of a connection
		 *
		 * A client will have one of these connections connected to a server. A server will have many, each one
		 * connected to a different client. A connection between a server and a particular client will only have
		 * one Connection instance for each.
		 *
		 * @author Mark Grimes (kknb1056@gmail.com)
		 * @date 30/Sep/2014
		 */
		class Connection : public comm::IConnection
		{
		public:
			typedef websocketpp::connection<websocketpp::config::asio>::ptr connection_ptr;
			typedef websocketpp::connection<websocketpp::config::asio>::message_ptr message_ptr;
			//typedef websocketpp::client<websocketpp::config::asio>::connection_ptr connection_ptr;
		public:
			Connection( connection_ptr pConnection );
			Connection( connection_ptr pConnection, std::function<void(const std::string&)>& infoHandler, std::function<std::string(const std::string&)>& requestHandler );
			virtual ~Connection();

			virtual void sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler ) override;
			virtual void sendInfo( const std::string& message ) override;
			virtual void setInfoHandler( std::function<void(const std::string&)> infoHandler ) override;
			virtual void setInfoHandler( std::function<void(const std::string&,comm::IConnection*)> infoHandler ) override;
			virtual void setRequestHandler( std::function<std::string(const std::string&)> requestHandler ) override;
			virtual void setRequestHandler( std::function<std::string(const std::string&,comm::IConnection*)> requestHandler ) override;

			connection_ptr& underlyingPointer();
			void close();
		private:
			connection_ptr pConnection_;
			std::function<void(const std::string&)> infoHandler_; ///< Only one info handler will be non-null
			std::function<void(const std::string&,comm::IConnection*)> infoHandlerAdvanced_;
			std::function<std::string(const std::string&)> requestHandler_; ///< Only one request handler will be non-null
			std::function<std::string(const std::string&,comm::IConnection*)> requestHandlerAdvanced_;
			/// This keeps track of the user references and associated handler for all requests
			/// sent but without a response received.
//			std::list< std::pair<comm::impl::Message::UserReference,std::function<void(const std::string&)> > > responseHandlers_;
			/// This is used as the user reference for the next request. If it gets close
			/// to overflowing then responseHandlers_ is checked for free numbers. If ever
			/// there are no responses pending it gets reset to zero.
//			std::atomic<comm::impl::Message::UserReference> availableUserReference_;
			comm::impl::UniqueTokenStorage<std::function<void(const std::string&)>,comm::impl::Message::UserReference> responseHandlers_;

			//
			// All the event handlers
			//
			void on_message( websocketpp::connection_hdl hdl, message_ptr msg );
			void on_open( websocketpp::connection_hdl hdl );
			void on_close( websocketpp::connection_hdl hdl );
			void on_interrupt( websocketpp::connection_hdl hdl );
		};

	} // end of namespace impl
} // end of namespace comm

#endif // end of ifndef comm_impl_Connection_h
