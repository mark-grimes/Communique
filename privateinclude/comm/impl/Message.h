#ifndef comm_impl_Message_h
#define comm_impl_Message_h

#include <functional>

#define _WEBSOCKETPP_CPP11_STL_ // Make sure websocketpp uses c++11 features in preference to boost ones
#include <websocketpp/connection.hpp>
#include <websocketpp/config/asio.hpp>

namespace comm
{

	namespace impl
	{
		/** @brief An abstraction layer of the message header.
		 *
		 * Handles getting the complete WebSocket message and decoding the Communique header and providing the
		 * body. The Communique header specifies whether the message is a new request or a response to a request,
		 * and how to match up responses to requests. It is separate to the WebSocket header - at the WebSocket
		 * level it is considered part of the message body.
		 *
		 * @author Mark Grimes (kknb1056@gmail.com)
		 * @date 03/Oct/2014
		 */
		class Message
		{
		public:
			typedef websocketpp::connection<websocketpp::config::asio>::ptr connection_ptr;
			typedef websocketpp::connection<websocketpp::config::asio>::message_ptr message_ptr;
			typedef uint32_t UserReference;
			enum MessageType { REQUEST, RESPONSE, INFO, REQUESTERROR }; ///< Note that this is currently packed into a char, so don't extend more than 16 types
		public:
			Message( message_ptr pMessage );
			Message( connection_ptr pConnection, const std::string& messageBody, MessageType type, UserReference userReference );
			virtual ~Message();

			MessageType type() const;
			UserReference userReference() const;
			std::string messageBody() const;
			const std::string& fullMessage() const;
			message_ptr websocketppMessage();
		private:
			message_ptr pFullMessage_;
		};

	} // end of namespace impl
} // end of namespace comm

#endif // end of ifndef comm_impl_Message_h
