#include <communique/impl/Connection.h>
#include <future>
#include "communique/impl/Message.h"

communique::impl::Connection::Connection( connection_ptr pConnection )
	: pConnection_(pConnection)
{
	pConnection_->set_message_handler( std::bind( &communique::impl::Connection::on_message, this, std::placeholders::_1, std::placeholders::_2 ) );
//	pConnection_->set_open_handler( std::bind( &communique::impl::Connection::on_open, this, std::placeholders::_1 ) );
//	pConnection_->set_close_handler( std::bind( &communique::impl::Connection::on_close, this, std::placeholders::_1 ) );
//	pConnection_->set_interrupt_handler( std::bind( &communique::impl::Connection::on_interrupt, this, std::placeholders::_1 ) );
}

communique::impl::Connection::Connection( connection_ptr pConnection, std::function<void(const std::string&)>& infoHandler, std::function<std::string(const std::string&)>& requestHandler )
	: pConnection_(pConnection)
{
	pConnection_->set_message_handler( std::bind( &communique::impl::Connection::on_message, this, std::placeholders::_1, std::placeholders::_2 ) );
//	pConnection_->set_open_handler( std::bind( &communique::impl::Connection::on_open, this, std::placeholders::_1 ) );
//	pConnection_->set_close_handler( std::bind( &communique::impl::Connection::on_close, this, std::placeholders::_1 ) );
//	pConnection_->set_interrupt_handler( std::bind( &communique::impl::Connection::on_interrupt, this, std::placeholders::_1 ) );
	setInfoHandler( infoHandler );
	setRequestHandler( requestHandler );
}

communique::impl::Connection::Connection( connection_ptr pConnection, std::function<void(const std::string&,communique::IConnection*)>& infoHandler, std::function<std::string(const std::string&,communique::IConnection*)>& requestHandler )
	: pConnection_(pConnection)
{
	pConnection_->set_message_handler( std::bind( &communique::impl::Connection::on_message, this, std::placeholders::_1, std::placeholders::_2 ) );
	setInfoHandler( infoHandler );
	setRequestHandler( requestHandler );
}

communique::impl::Connection::~Connection()
{

}

void communique::impl::Connection::sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler )
{
	// This call will give me a unique token that I can use to retrieve the handler
	// later. I'll transmit this token to the other side of the connection so that
	// they use it in the response. Once I get the response with the token in it
	// I can use the token to retrieve the correct handler.
	communique::impl::Message::UserReference userReference=responseHandlers_.push( responseHandler );

	communique::impl::Message newMessage( pConnection_, message, communique::impl::Message::REQUEST, userReference );
	pConnection_->send( newMessage.websocketppMessage() );
}

void communique::impl::Connection::sendInfo( const std::string& message )
{
	communique::impl::Message newMessage( pConnection_, message, communique::impl::Message::INFO, 0 );
	pConnection_->send( newMessage.websocketppMessage() );
}

void communique::impl::Connection::setInfoHandler( std::function<void(const std::string&)> infoHandler )
{
	// Wrap in a function that drops the connection argument
	infoHandler_=std::bind( infoHandler, std::placeholders::_1 );;
}

void communique::impl::Connection::setInfoHandler( std::function<void(const std::string&,communique::IConnection*)> infoHandler )
{
	infoHandler_=infoHandler;
}

void communique::impl::Connection::setRequestHandler( std::function<std::string(const std::string&)> requestHandler )
{
	// Wrap in a function that drops the connection argument
	requestHandler_=std::bind( requestHandler, std::placeholders::_1 );
}

void communique::impl::Connection::setRequestHandler( std::function<std::string(const std::string&,communique::IConnection*)> requestHandler )
{
	requestHandler_=requestHandler;
}

communique::impl::Connection::connection_ptr& communique::impl::Connection::underlyingPointer()
{
	return pConnection_;
}

bool communique::impl::Connection::isConnected()
{
	//
	// Make sure the connection is not in a transitioning state. If it is, wait until it finishes.
	// Not fussed if the connection is "closing" because I know it won't transition to state that
	// would require returning true.
	//
	while( pConnection_->get_state()==websocketpp::session::state::connecting )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );
	}

	return pConnection_->get_state()==websocketpp::session::state::open;
}

bool communique::impl::Connection::isDisconnected()
{
	//
	// Make sure the connection is not in a transitioning state. If it is, wait until it finishes.
	// This isn't quite the same as "!isConnected()" because this method can be used to block
	// while a client disconnects, whereas isConnected() only blocks while the client is connecting.
	//
	while( pConnection_->get_state()==websocketpp::session::state::closing )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );
	}

	return pConnection_->get_state()==websocketpp::session::state::closed;
}

void communique::impl::Connection::close()
{
	//
	// If the connection is open, tell it to close. If the connection is in the "connecting"
	// state the isConnected call blocks until it changes to something else.
	//
	if( isConnected() )
	{
		websocketpp::lib::error_code errorCode;

		pConnection_->close( websocketpp::close::status::normal, "Had enough. Bye.", errorCode );

		//if( errorCode ) pImple_->client_.get_elog().write( websocketpp::log::alevel::app, errorCode.message() );
		if( errorCode ) std::cerr << "communique::impl::Connection::close() - " << errorCode.message() << std::endl;
	}
}

void communique::impl::Connection::on_message( websocketpp::connection_hdl hdl, communique::impl::Connection::message_ptr msg )
{
//	std::cout << "Received message '" << msg->get_payload() << "'" << std::flush;
	communique::impl::Message receivedMessage( msg );
//	std::cout << " type=" << receivedMessage.type() << " userReference=" << receivedMessage.userReference() << std::endl;

	if( receivedMessage.type()==communique::impl::Message::INFO )
	{
		if( infoHandler_ ) infoHandler_( receivedMessage.messageBody(), this );
		else std::cout << "Ignoring info message '" << receivedMessage.messageBody() << "'" << std::endl;
	}
	else if( receivedMessage.type()==communique::impl::Message::REQUEST )
	{
		if( requestHandler_ )
		{
			std::async( std::launch::async, [ this, receivedMessage ]() // Copy receivedMessage by value because internally it holds a shared_ptr to the message
			{
				std::string handlerResponse;
				communique::impl::Message::MessageType responseType=communique::impl::Message::RESPONSE;
				try
				{
					handlerResponse=requestHandler_( receivedMessage.messageBody(), this );
				}
				catch( std::exception& error )
				{
					handlerResponse=error.what();
					responseType=communique::impl::Message::REQUESTERROR;
				}
				catch(...)
				{
					handlerResponse="Unknown exception";
					responseType=communique::impl::Message::REQUESTERROR;
				}
				communique::impl::Message newMessage( pConnection_, handlerResponse, responseType, receivedMessage.userReference() );
				// Send the rest of the message with the header stripped off first, and use the
				// return from the handler
				pConnection_->send( newMessage.websocketppMessage() );
			} );
		}
		else
		{
			std::cout << "Ignoring request message '" << receivedMessage.messageBody() << "'" << std::endl;
			communique::impl::Message newMessage( pConnection_, "No request handler set", communique::impl::Message::REQUESTERROR, receivedMessage.userReference() );
			pConnection_->send( newMessage.websocketppMessage() );
		}
	}
	else if( receivedMessage.type()==communique::impl::Message::RESPONSE )
	{
		// This will be the response to a request that I've sent out, so
		// I need to search for the handler that was stored when the message
		// was sent.

		std::function<void(const std::string&)> responseHandler;
		if( responseHandlers_.pop( receivedMessage.userReference(), responseHandler ) )
		{
			std::async( std::launch::async, responseHandler, receivedMessage.messageBody() );
		}
		else // response handler was not found in the list for the userReference
		{
			// This should never happen for well behaved clients/servers. I guess an attacker
			// could try crafting messages to force this.
			std::cout << "Ignoring response message '" << receivedMessage.messageBody() << "' because no handler found" << std::endl;
		}
	}
}

//void communique::impl::Connection::on_open( websocketpp::connection_hdl hdl )
//{
//
//}
//
//void communique::impl::Connection::on_close( websocketpp::connection_hdl hdl )
//{
//
//}
//
//void communique::impl::Connection::on_interrupt( websocketpp::connection_hdl hdl )
//{
//
//}
