#include "comm/impl/Message.h"

comm::impl::Message::Message( message_ptr pMessage )
	: pFullMessage_( pMessage )
{
	if( pMessage->get_payload().size()<5 ) throw std::runtime_error("Received a websocket message that is too small to be a Communique message");
}

comm::impl::Message::Message( connection_ptr pConnection, const std::string& messageBody, MessageType type, UserReference userReference )
	: pFullMessage_( pConnection->get_message(websocketpp::frame::opcode::BINARY,messageBody.size()+sizeof(UserReference)+sizeof(char)) )
{
	// Write directly into the payload. It should be the correct size from the initialiser list
	std::string& payload=pFullMessage_->get_raw_payload();
	payload.resize(messageBody.size()+sizeof(UserReference)+sizeof(char));

	// Need to convert userReference to network byte order
	UserReference userNetorder=htonl(userReference); // If this doesn't compile try "#include <arpa/inet.h>"
	payload[0]=static_cast<char>( type ); // Write the message type in the first character
	payload.replace( 1, 4, reinterpret_cast<char*>(&userNetorder), 4 ); // Write the user reference in the next 4
	payload.replace( 5, std::string::npos, messageBody ); // Then put the message in everything after that
}

comm::impl::Message::~Message()
{

}

comm::impl::Message::MessageType comm::impl::Message::type() const
{
	return static_cast<MessageType>( pFullMessage_->get_payload()[0] );
}

comm::impl::Message::UserReference comm::impl::Message::userReference() const
{
	return ntohl( *reinterpret_cast<const UserReference*>(&pFullMessage_->get_payload()[1]) );
}

std::string comm::impl::Message::messageBody() const
{
	return pFullMessage_->get_payload().substr( 5 );
}

const std::string& comm::impl::Message::fullMessage() const
{
	return pFullMessage_->get_payload();
}

comm::impl::Message::message_ptr comm::impl::Message::websocketppMessage()
{
	return pFullMessage_;
}
