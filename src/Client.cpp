#include "comm/Client.h"

#include <mutex>
#include <future>
#include "comm/impl/Exceptions.h"

#define _WEBSOCKETPP_CPP11_STL_ // Make sure websocketpp uses c++11 features in preference to boost ones
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>
#include "comm/impl/Connection.h"

//
// Declaration of the pimple
//
namespace comm
{
	class ClientPrivateMembers
	{
	public:
		typedef websocketpp::client<websocketpp::config::asio> client_type;
		std::thread ioThread_;
		client_type client_;
		std::auto_ptr<comm::impl::Connection> pConnection_;

		std::function<void(const std::string&)> infoHandler_;
		std::function<void(const std::string&,comm::IConnection*)> infoHandlerAdvanced_;
		std::function<std::string(const std::string&)> requestHandler_;
		std::function<std::string(const std::string&,comm::IConnection*)> requestHandlerAdvanced_;


		std::shared_ptr<boost::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl );
//		void on_message( websocketpp::connection_hdl hdl, client_type::message_ptr msg );
//		void on_open( websocketpp::connection_hdl hdl );
//		void on_close( websocketpp::connection_hdl hdl );
//		void on_interrupt( websocketpp::connection_hdl hdl );
	};
}

comm::Client::Client()
	: pImple_( new ClientPrivateMembers )
{
	pImple_->client_.set_access_channels(websocketpp::log::alevel::none);
	//pImple_->client_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	pImple_->client_.set_error_channels(websocketpp::log::elevel::none);
	pImple_->client_.init_asio();
//	pImple_->client_.set_tls_init_handler( std::bind( &ClientPrivateMembers::on_tls_init, pImple_.get(), std::placeholders::_1 ) );
//	pImple_->client_.set_message_handler( std::bind( &ClientPrivateMembers::on_message, pImple_.get(), std::placeholders::_1, std::placeholders::_2 ) );
//	pImple_->client_.set_open_handler( std::bind( &ClientPrivateMembers::on_open, pImple_.get(), std::placeholders::_1 ) );
//	pImple_->client_.set_close_handler( std::bind( &ClientPrivateMembers::on_close, pImple_.get(), std::placeholders::_1 ) );
//	pImple_->client_.set_interrupt_handler( std::bind( &ClientPrivateMembers::on_interrupt, pImple_.get(), std::placeholders::_1 ) );
}

comm::Client::Client( Client&& otherClient ) noexcept
	: pImple_( std::move(otherClient.pImple_) )
{
	// No operation, everything done in initialiser list
}

comm::Client::~Client()
{
	try
	{
		// If std::move is used then pImple_ can be null, in which
		// case I don't want to do any cleanup.
		if( pImple_ ) disconnect();
	}
	catch(...) { /* Make sure no exceptions propagate out */ }
}

std::shared_ptr<boost::asio::ssl::context> comm::ClientPrivateMembers::on_tls_init( websocketpp::connection_hdl hdl )
{
	std::cout << "init_tls called" << std::endl;
	websocketpp::lib::shared_ptr<boost::asio::ssl::context> pContext( new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1) );
	pContext->set_options( boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use );
	//pContext->set_password_callback( boost::bind( &server::get_password, this ) );
	pContext->use_certificate_chain_file( "/Users/phmag/Documents/EclipseWorkspaces/temp/keys/server.pem" );
	pContext->use_private_key_file( "/Users/phmag/Documents/EclipseWorkspaces/temp/keys/server.pem", boost::asio::ssl::context::pem );
	pContext->load_verify_file( "/Users/phmag/Documents/EclipseWorkspaces/temp/keys/rootCA.pem" );
	pContext->use_tmp_dh_file( "/Users/phmag/Documents/EclipseWorkspaces/temp/dh512.pem" );
	//pContext->set_verify_mode( boost::asio::ssl::verify_peer/* | boost::asio::ssl::verify_fail_if_no_peer_cert*/ );

	return pContext;
}

void comm::Client::connect( const std::string& URI )
{
	websocketpp::lib::error_code errorCode;
	pImple_->pConnection_.reset( new comm::impl::Connection( pImple_->client_.get_connection( URI, errorCode ), pImple_->infoHandler_, pImple_->requestHandler_ ) );
	// If "advanced" handlers have beed set (just has IConnection* as additional argument)
	// change the handler to these.
	if( pImple_->infoHandlerAdvanced_ ) pImple_->pConnection_->setInfoHandler(pImple_->infoHandlerAdvanced_);
	if( pImple_->requestHandlerAdvanced_ ) pImple_->pConnection_->setRequestHandler(pImple_->requestHandlerAdvanced_);

	if( errorCode )
	{
		pImple_->client_.get_alog().write(websocketpp::log::alevel::app,errorCode.message());
		throw comm::impl::Exception( errorCode.message() );
	}

	pImple_->client_.connect( pImple_->pConnection_->underlyingPointer() );
	pImple_->ioThread_=std::thread( &ClientPrivateMembers::client_type::run, &pImple_->client_ );
}

void comm::Client::disconnect()
{
	if( pImple_->pConnection_.get()!=nullptr ) pImple_->pConnection_->close();
	if( pImple_->ioThread_.joinable() ) pImple_->ioThread_.join();
}

void comm::Client::sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler )
{
	if( !pImple_->pConnection_.get() ) throw comm::impl::Exception( "No connection" );
	pImple_->pConnection_->sendRequest( message, responseHandler );
}

void comm::Client::sendInfo( const std::string& message )
{
	if( !pImple_->pConnection_.get() ) throw comm::impl::Exception( "No connection" );
	pImple_->pConnection_->sendInfo( message );
}

void comm::Client::setInfoHandler( std::function<void(const std::string&)> infoHandler )
{
	pImple_->infoHandler_=infoHandler;
	pImple_->infoHandlerAdvanced_=nullptr;
	if( pImple_->pConnection_.get() ) // don't know why I need ".get()". C++11 standard should allow auto_ptr comparison
	{
		pImple_->pConnection_->setInfoHandler( infoHandler );
	}
}

void comm::Client::setInfoHandler( std::function<void(const std::string&,comm::IConnection*)> infoHandler )
{
	pImple_->infoHandler_=nullptr;
	pImple_->infoHandlerAdvanced_=infoHandler;
	if( pImple_->pConnection_.get() ) // don't know why I need ".get()". C++11 standard should allow auto_ptr comparison
	{
		pImple_->pConnection_->setInfoHandler( infoHandler );
	}
}

void comm::Client::setRequestHandler( std::function<std::string(const std::string&)> requestHandler )
{
	pImple_->requestHandler_=requestHandler;
	pImple_->requestHandlerAdvanced_=nullptr;
	if( pImple_->pConnection_.get() )
	{
		pImple_->pConnection_->setRequestHandler( requestHandler );
	}
}

void comm::Client::setRequestHandler( std::function<std::string(const std::string&,comm::IConnection*)> requestHandler )
{
	pImple_->requestHandler_=nullptr;
	pImple_->requestHandlerAdvanced_=requestHandler;
	if( pImple_->pConnection_.get() )
	{
		pImple_->pConnection_->setRequestHandler( requestHandler );
	}
}

//void comm::ClientPrivateMembers::on_message( websocketpp::connection_hdl hdl, client_type::message_ptr msg )
//{
//	comm::MessageHeader messageHeader;
//	messageHeader.ParseFromString( msg->get_payload() );
//
//	if( messageHeader.type()==comm::MessageHeader::INFO )
//	{
//		if( infoHandler_ ) infoHandler_( msg->get_payload().substr(messageHeader.ByteSize()) );
//	}
//	else if( messageHeader.type()==comm::MessageHeader::REQUEST )
//	{
//		if( requestHandler_ && messageHeader.has_user_reference() )
//		{
//			// Record where the message header stops and the body starts
//			size_t messageBodyOffset=messageHeader.ByteSize();
//
//			// Reuse the message header and stream out as a string
//			messageHeader.set_type( comm::MessageHeader::RESPONSE );
//			// The user reference will already be set correctly
//			std::string headerString;
//			messageHeader.SerializeToString( &headerString );
//
//			std::async( [ this, hdl, msg, messageBodyOffset, headerString ]()
//			{
//				// Send the rest of the message with the header stripped off first, and use the
//				// return from the handler
//				client_.send( hdl, headerString+requestHandler_( msg->get_payload().substr(messageBodyOffset) ), websocketpp::frame::opcode::BINARY );
//			} );
//		}
//	}
//	else if( messageHeader.type()==comm::MessageHeader::RESPONSE )
//	{
//		// This will be the response to a request that I've sent out, so
//		// I need to search for the handler that was stored when the message
//		// was sent.
//
//		// TODO - implement
//	}
//}
//
//void comm::ClientPrivateMembers::on_open( websocketpp::connection_hdl hdl )
//{
//	client_.send( hdl, "What day is it?", websocketpp::frame::opcode::TEXT );
//}
//
//void comm::ClientPrivateMembers::on_close( websocketpp::connection_hdl hdl )
//{
//	std::cout << "Connection has closed" << std::endl;
//}
//
//void comm::ClientPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
//{
//	std::cout << "Connection has been interrupted" << std::endl;
//}
