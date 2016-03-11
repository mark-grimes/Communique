#include "communique/Client.h"

#include <mutex>
#include <future>
#include "communique/impl/Exceptions.h"

#define _WEBSOCKETPP_CPP11_STL_ // Make sure websocketpp uses c++11 features in preference to boost ones
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>
#include "communique/impl/Connection.h"
#include "communique/impl/TLSHandler.h"

//
// Declaration of the pimple
//
namespace communique
{
	class ClientPrivateMembers
	{
	public:
		typedef websocketpp::client<websocketpp::config::asio_tls> client_type;

		ClientPrivateMembers() : tlsHandler_(client_.get_alog()) { /*No operation besides initialiser list*/ }
		client_type client_;
		std::thread ioThread_;
		std::shared_ptr<communique::impl::Connection> pConnection_; // Needs to be shared rather than unique because it's passed to handlers
		communique::impl::TLSHandler tlsHandler_;

		std::function<void(const std::string&,std::weak_ptr<communique::IConnection>)> infoHandler_;
		std::function<std::string(const std::string&,std::weak_ptr<communique::IConnection>)> requestHandler_;

		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );
	};
}

communique::Client::Client()
	: pImple_( new ClientPrivateMembers )
{
	pImple_->client_.set_access_channels(websocketpp::log::alevel::none);
	//pImple_->client_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	pImple_->client_.set_error_channels(websocketpp::log::elevel::none);
	pImple_->client_.set_tls_init_handler( std::bind( &communique::impl::TLSHandler::on_tls_init, &pImple_->tlsHandler_, std::placeholders::_1 ) );
	pImple_->client_.init_asio();
	pImple_->client_.set_open_handler( std::bind( &ClientPrivateMembers::on_open, pImple_.get(), std::placeholders::_1 ) );
	pImple_->client_.set_close_handler( std::bind( &ClientPrivateMembers::on_close, pImple_.get(), std::placeholders::_1 ) );
	pImple_->client_.set_interrupt_handler( std::bind( &ClientPrivateMembers::on_interrupt, pImple_.get(), std::placeholders::_1 ) );
}

communique::Client::Client( Client&& otherClient ) noexcept
	: pImple_( std::move(otherClient.pImple_) )
{
	// No operation, everything done in initialiser list
}

communique::Client::~Client()
{
	try
	{
		// If std::move is used then pImple_ can be null, in which
		// case I don't want to do any cleanup.
		if( pImple_ ) disconnect();
	}
	catch(...) { /* Make sure no exceptions propagate out */ }
}

void communique::Client::connect( const std::string& URI )
{
	websocketpp::lib::error_code errorCode;
	auto pWebPPConnection=pImple_->client_.get_connection( URI, errorCode );
	if( errorCode.value()!=0 ) throw std::runtime_error( "Unable to get the websocketpp connection - "+errorCode.message() );
	pImple_->pConnection_=std::make_shared<communique::impl::Connection>( pWebPPConnection, pImple_->infoHandler_, pImple_->requestHandler_ );

	if( errorCode )
	{
		pImple_->client_.get_alog().write(websocketpp::log::alevel::app,errorCode.message());
		throw communique::impl::Exception( errorCode.message() );
	}

	pImple_->client_.connect( pImple_->pConnection_->underlyingPointer() );
	pImple_->ioThread_=std::thread( &ClientPrivateMembers::client_type::run, &pImple_->client_ );
}

bool communique::Client::isConnected()
{
	if( !pImple_->pConnection_ ) return false;
	return pImple_->pConnection_->isConnected();
}

bool communique::Client::isDisconnected()
{
	if( !pImple_->pConnection_ ) return true;
	return pImple_->pConnection_->isDisconnected();
}

void communique::Client::disconnect()
{
	if( pImple_->pConnection_ ) pImple_->pConnection_->close();
	if( pImple_->ioThread_.joinable() ) pImple_->ioThread_.join();
}

void communique::Client::setCertificateChainFile( const std::string& filename )
{
	pImple_->tlsHandler_.setCertificateChainFile(filename);
}

void communique::Client::setPrivateKeyFile( const std::string& filename )
{
	pImple_->tlsHandler_.setPrivateKeyFile(filename);
}

void communique::Client::setVerifyFile( const std::string& filename )
{
	pImple_->tlsHandler_.setVerifyFile(filename);
}

void communique::Client::sendRequest( const std::string& message, std::function<void(const std::string&)> responseHandler )
{
	if( !pImple_->pConnection_ ) throw communique::impl::Exception( "No connection" );
	pImple_->pConnection_->sendRequest( message, responseHandler );
}

void communique::Client::sendInfo( const std::string& message )
{
	if( !pImple_->pConnection_ ) throw communique::impl::Exception( "No connection" );
	pImple_->pConnection_->sendInfo( message );
}

void communique::Client::setInfoHandler( std::function<void(const std::string&)> infoHandler )
{
	// Wrap in a function that drops the connection argument
	pImple_->infoHandler_=std::bind( infoHandler, std::placeholders::_1 );
	if( pImple_->pConnection_ )
	{
		pImple_->pConnection_->setInfoHandler( pImple_->infoHandler_ );
	}
}

void communique::Client::setInfoHandler( std::function<void(const std::string&,std::weak_ptr<communique::IConnection>)> infoHandler )
{
	pImple_->infoHandler_=infoHandler;
	if( pImple_->pConnection_ )
	{
		pImple_->pConnection_->setInfoHandler( pImple_->infoHandler_ );
	}
}

void communique::Client::setRequestHandler( std::function<std::string(const std::string&)> requestHandler )
{
	// Wrap in a function that drops the connection argument
	pImple_->requestHandler_=std::bind( requestHandler, std::placeholders::_1 );
	if( pImple_->pConnection_ )
	{
		pImple_->pConnection_->setRequestHandler( pImple_->requestHandler_ );
	}
}

void communique::Client::setRequestHandler( std::function<std::string(const std::string&,std::weak_ptr<communique::IConnection>)> requestHandler )
{
	pImple_->requestHandler_=requestHandler;
	if( pImple_->pConnection_ )
	{
		pImple_->pConnection_->setRequestHandler( pImple_->requestHandler_ );
	}
}

void communique::Client::setErrorLogLocation( std::ostream& outputStream )
{
	pImple_->client_.get_elog().set_ostream( &outputStream );
}

void communique::Client::setErrorLogLevel( uint32_t level )
{
	pImple_->client_.set_error_channels(level);
}

void communique::Client::setAccessLogLocation( std::ostream& outputStream )
{
	pImple_->client_.get_alog().set_ostream( &outputStream );
}

void communique::Client::setAccessLogLevel( uint32_t level )
{
	pImple_->client_.set_access_channels(level);
}

void communique::ClientPrivateMembers::on_open( websocketpp::connection_hdl hdl )
{
}

void communique::ClientPrivateMembers::on_close( websocketpp::connection_hdl hdl )
{
}

void communique::ClientPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has been interrupted" << std::endl;
}
