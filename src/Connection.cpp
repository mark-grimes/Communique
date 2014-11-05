#include <comm/impl/Connection.h>
#include <future>
#include "comm/impl/Message.h"

comm::impl::Connection::Connection( connection_ptr pConnection )
	: pConnection_(pConnection)
{
	pConnection_->set_message_handler( std::bind( &comm::impl::Connection::on_message, this, std::placeholders::_1, std::placeholders::_2 ) );
//	pConnection_->set_open_handler( std::bind( &comm::impl::Connection::on_open, this, std::placeholders::_1 ) );
//	pConnection_->set_close_handler( std::bind( &comm::impl::Connection::on_close, this, std::placeholders::_1 ) );
//	pConnection_->set_interrupt_handler( std::bind( &comm::impl::Connection::on_interrupt, this, std::placeholders::_1 ) );
}

comm::impl::Connection::Connection( connection_ptr pConnection, std::function<void(const std::string&)>& infoHandler, std::function<std::string(const std::string&)>& requestHandler )
	: pConnection_(pConnection)
{
	pConnection_->set_message_handler( std::bind( &comm::impl::Connection::on_message, this, std::placeholders::_1, std::placeholders::_2 ) );
//	pConnection_->set_open_handler( std::bind( &comm::impl::Connection::on_open, this, std::placeholders::_1 ) );
//	pConnection_->set_close_handler( std::bind( &comm::impl::Connection::on_close, this, std::placeholders::_1 ) );
//	pConnection_->set_interrupt_handler( std::bind( &comm::impl::Connection::on_interrupt, this, std::placeholders::_1 ) );
	setInfoHandler( infoHandler );
	setRequestHandler( requestHandler );
}

comm::impl::Connection::~Connection()
{

}

void comm::impl::Connection::sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler )
{
	// This call will give me a unique token that I can use to retrieve the handler
	// later. I'll transmit this token to the other side of the connection so that
	// they use it in the response. Once I get the response with the token in it
	// I can use the token to retrieve the correct handler.
	comm::impl::Message::UserReference userReference=responseHandlers_.store( responseHandler );

	comm::impl::Message newMessage( pConnection_, message, comm::impl::Message::REQUEST, userReference );
	pConnection_->send( newMessage.websocketppMessage() );
}

void comm::impl::Connection::sendInfo( const std::string& message )
{
	comm::impl::Message newMessage( pConnection_, message, comm::impl::Message::INFO, 0 );
	pConnection_->send( newMessage.websocketppMessage() );
}

void comm::impl::Connection::setInfoHandler( std::function<void(const std::string&)> infoHandler )
{
	infoHandler_=infoHandler;
	infoHandlerAdvanced_=nullptr;
}

void comm::impl::Connection::setInfoHandler( std::function<void(const std::string&,comm::IConnection*)> infoHandler )
{
	infoHandler_=nullptr;
	infoHandlerAdvanced_=infoHandler;
}

void comm::impl::Connection::setRequestHandler( std::function<std::string(const std::string&)> requestHandler )
{
	requestHandler_=requestHandler;
	requestHandlerAdvanced_=nullptr;
}

void comm::impl::Connection::setRequestHandler( std::function<std::string(const std::string&,comm::IConnection*)> requestHandler )
{
	requestHandler_=nullptr;
	requestHandlerAdvanced_=requestHandler;
}

comm::impl::Connection::connection_ptr& comm::impl::Connection::underlyingPointer()
{
	return pConnection_;
}

void comm::impl::Connection::close()
{
	if( pConnection_->get_state()==websocketpp::session::state::open )
	{
		websocketpp::lib::error_code errorCode;

		pConnection_->close( websocketpp::close::status::normal, "Had enough. Bye.", errorCode );

		//if( errorCode ) pImple_->client_.get_elog().write( websocketpp::log::alevel::app, errorCode.message() );
		if( errorCode ) std::cerr << "comm::impl::Connection::close() - " << errorCode.message() << std::endl;
	}
}

void comm::impl::Connection::on_message( websocketpp::connection_hdl hdl, comm::impl::Connection::message_ptr msg )
{
//	std::cout << "Received message '" << msg->get_payload() << "'" << std::flush;
	comm::impl::Message receivedMessage( msg );
//	std::cout << " type=" << receivedMessage.type() << " userReference=" << receivedMessage.userReference() << std::endl;

	if( receivedMessage.type()==comm::impl::Message::INFO )
	{
		if( infoHandler_ ) infoHandler_( receivedMessage.messageBody() );
		else if( infoHandlerAdvanced_ ) infoHandlerAdvanced_( receivedMessage.messageBody(), this );
		else std::cout << "Ignoring info message '" << receivedMessage.messageBody() << "'" << std::endl;
	}
	else if( receivedMessage.type()==comm::impl::Message::REQUEST )
	{
		if( requestHandler_ || requestHandlerAdvanced_ )
		{
			std::async( [ this, receivedMessage ]() // Copy receivedMessage by value because internally it holds a shared_ptr to the message
			{
				std::string handlerResponse;
				comm::impl::Message::MessageType responseType=comm::impl::Message::RESPONSE;
				try
				{
					if( requestHandler_ ) handlerResponse=requestHandler_( receivedMessage.messageBody() );
					else handlerResponse=requestHandlerAdvanced_( receivedMessage.messageBody(), this );
				}
				catch( std::exception& error )
				{
					handlerResponse=error.what();
					responseType=comm::impl::Message::REQUESTERROR;
				}
				catch(...)
				{
					handlerResponse="Unknown exception";
					responseType=comm::impl::Message::REQUESTERROR;
				}
				comm::impl::Message newMessage( pConnection_, handlerResponse, responseType, receivedMessage.userReference() );
				// Send the rest of the message with the header stripped off first, and use the
				// return from the handler
				pConnection_->send( newMessage.websocketppMessage() );
			} );
		}
		else
		{
			std::cout << "Ignoring request message '" << receivedMessage.messageBody() << "'" << std::endl;
			comm::impl::Message newMessage( pConnection_, "No request handler set", comm::impl::Message::REQUESTERROR, receivedMessage.userReference() );
			pConnection_->send( newMessage.websocketppMessage() );
		}
	}
	else if( receivedMessage.type()==comm::impl::Message::RESPONSE )
	{
		// This will be the response to a request that I've sent out, so
		// I need to search for the handler that was stored when the message
		// was sent.

		std::function<void(const std::string&)> responseHandler;
		if( responseHandlers_.retrieve( receivedMessage.userReference(), responseHandler ) )
		{
			std::async( responseHandler, receivedMessage.messageBody() );
		}
		else // response handler was not found in the list for the userReference
		{
			// This should never happen for well behaved clients/servers. I guess an attacker
			// could try crafting messages to force this.
			std::cout << "Ignoring response message '" << receivedMessage.messageBody() << "' because no handler found" << std::endl;
		}
	}
}

//void comm::impl::Connection::on_open( websocketpp::connection_hdl hdl )
//{
//
//}
//
//void comm::impl::Connection::on_close( websocketpp::connection_hdl hdl )
//{
//
//}
//
//void comm::impl::Connection::on_interrupt( websocketpp::connection_hdl hdl )
//{
//
//}
