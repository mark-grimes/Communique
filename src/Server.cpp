#include "communique/Server.h"

#define _WEBSOCKETPP_CPP11_STL_ // Make sure websocketpp uses c++11 features in preference to boost ones
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio.hpp>
#include <list>
#include "communique/impl/Connection.h"


//
// Declaration of the pimple
//
namespace communique
{
	class ServerPrivateMembers
	{
	public:
		typedef websocketpp::server<websocketpp::config::asio_tls> server_type;
		server_type server_;
		std::thread ioThread_;
		std::list< std::shared_ptr<communique::impl::Connection> > currentConnections_;
		mutable std::mutex currentConnectionsMutex_;
		std::string certificateChainFile_;
		std::string privateKeyFile_;
		std::string verifyFile_;
		std::string diffieHellmanParamsFile_;

		websocketpp::lib::shared_ptr<boost::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl );
		//void on_message( websocketpp::connection_hdl hdl, server_type::message_ptr msg );
		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );

		std::function<void(const std::string&)> defaultInfoHandler_;
		std::function<void(const std::string&,communique::IConnection*)> defaultInfoHandlerAdvanced_;
		std::function<std::string(const std::string&)> defaultRequestHandler_;
		std::function<std::string(const std::string&,communique::IConnection*)> defaultRequestHandlerAdvanced_;
	};
}

communique::Server::Server()
	: pImple_( new ServerPrivateMembers )
{
	pImple_->server_.set_access_channels(websocketpp::log::alevel::none);
	//pImple_->server_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	pImple_->server_.set_error_channels(websocketpp::log::elevel::none);
	pImple_->server_.set_tls_init_handler( std::bind( &ServerPrivateMembers::on_tls_init, pImple_.get(), std::placeholders::_1 ) );
	pImple_->server_.init_asio();
	//pImple_->server_.set_message_handler( std::bind( &ServerPrivateMembers::on_message, pImple_.get(), std::placeholders::_1, std::placeholders::_2 ) );
	pImple_->server_.set_open_handler( std::bind( &ServerPrivateMembers::on_open, pImple_.get(), std::placeholders::_1 ) );
	pImple_->server_.set_close_handler( std::bind( &ServerPrivateMembers::on_close, pImple_.get(), std::placeholders::_1 ) );
	pImple_->server_.set_interrupt_handler( std::bind( &ServerPrivateMembers::on_interrupt, pImple_.get(), std::placeholders::_1 ) );
}

communique::Server::~Server()
{
	try
	{
		stop();
	}
	catch(...) { /* Make sure no exceptions propagate out */ }
}

bool communique::Server::listen( size_t port )
{
	try
	{
		if( pImple_->ioThread_.joinable() ) stop(); // If already running stop the current IO

		pImple_->server_.listen(port);
		pImple_->server_.start_accept();

		pImple_->ioThread_=std::thread( &ServerPrivateMembers::server_type::run, &pImple_->server_ );

		return true;
	}
	catch( std::exception& error )
	{
		throw;
	}
	catch(...)
	{
		throw std::runtime_error( "Unknown exception in communique::Server::listen" );
	}
}

void communique::Server::stop()
{
	if( pImple_->server_.is_listening() ) pImple_->server_.stop_listening();

	{ // Block to limit lifetime of the lock_guard
		std::lock_guard<std::mutex> myMutex( pImple_->currentConnectionsMutex_ );
		for( auto& pConnection : pImple_->currentConnections_ )
		{
			//pImple_->server_.get_con_from_hdl(connection_hdl)->get_raw_socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			pConnection->close();
		}
	}

	if( pImple_->ioThread_.joinable() ) pImple_->ioThread_.join();
}

void communique::Server::setCertificateChainFile( const std::string& filename )
{
	pImple_->certificateChainFile_=filename;
}

void communique::Server::setPrivateKeyFile( const std::string& filename )
{
	pImple_->privateKeyFile_=filename;
}

void communique::Server::setVerifyFile( const std::string& filename )
{
	pImple_->verifyFile_=filename;
}

void communique::Server::setDiffieHellmanParamsFile( const std::string& filename )
{
	pImple_->diffieHellmanParamsFile_=filename;
}

void communique::Server::setDefaultInfoHandler( std::function<void(const std::string&)> infoHandler )
{
	pImple_->defaultInfoHandler_=infoHandler;
	pImple_->defaultInfoHandlerAdvanced_=nullptr;
}

void communique::Server::setDefaultInfoHandler( std::function<void(const std::string&,communique::IConnection*)> infoHandler )
{
	pImple_->defaultInfoHandler_=nullptr;
	pImple_->defaultInfoHandlerAdvanced_=infoHandler;
}

void communique::Server::setDefaultRequestHandler( std::function<const std::string(const std::string&)> requestHandler )
{
	pImple_->defaultRequestHandler_=requestHandler;
	pImple_->defaultRequestHandlerAdvanced_=nullptr;
}

void communique::Server::setDefaultRequestHandler( std::function<const std::string(const std::string&,communique::IConnection*)> requestHandler )
{
	pImple_->defaultRequestHandler_=nullptr;
	pImple_->defaultRequestHandlerAdvanced_=requestHandler;
}

std::vector<std::shared_ptr<communique::IConnection> > communique::Server::currentConnections()
{
	std::lock_guard<std::mutex> myMutex( pImple_->currentConnectionsMutex_ );
	return std::vector<std::shared_ptr<communique::IConnection> >( pImple_->currentConnections_.begin(), pImple_->currentConnections_.end() );
}

websocketpp::lib::shared_ptr<boost::asio::ssl::context> communique::ServerPrivateMembers::on_tls_init( websocketpp::connection_hdl hdl )
{
	websocketpp::lib::shared_ptr<boost::asio::ssl::context> pContext( new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1) );
	pContext->set_options( boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use );
	//pContext->set_password_callback( boost::bind( &server::get_password, this ) );
	if( !certificateChainFile_.empty() ) pContext->use_certificate_chain_file( certificateChainFile_ );
	if( !privateKeyFile_.empty() ) pContext->use_private_key_file( privateKeyFile_, boost::asio::ssl::context::pem );
	if( !verifyFile_.empty() )
	{
		pContext->set_verify_mode( boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert );
		pContext->load_verify_file( verifyFile_ );
	}
	if( !diffieHellmanParamsFile_.empty() ) pContext->use_tmp_dh_file( diffieHellmanParamsFile_ );

	return pContext;
}

//void communique::ServerPrivateMembers::on_message( websocketpp::connection_hdl hdl, server_type::message_ptr msg )
//{
//	std::cout << "Message is '" << msg->get_payload() << "'" << std::endl;
//}

void communique::ServerPrivateMembers::on_open( websocketpp::connection_hdl hdl )
{
	std::lock_guard<std::mutex> myMutex( currentConnectionsMutex_ );
	currentConnections_.emplace_back( new communique::impl::Connection( server_.get_con_from_hdl(hdl), defaultInfoHandler_, defaultRequestHandler_ ) );
	// If "advanced" handlers have beed set (just has IConnection* as additional argument)
	// change the handler to these.
	if( defaultInfoHandlerAdvanced_ ) currentConnections_.back()->setInfoHandler(defaultInfoHandlerAdvanced_);
	if( defaultRequestHandlerAdvanced_ ) currentConnections_.back()->setRequestHandler(defaultRequestHandlerAdvanced_);
}

void communique::ServerPrivateMembers::on_close( websocketpp::connection_hdl hdl )
{
	std::lock_guard<std::mutex> myMutex( currentConnectionsMutex_ );
	auto pRawConnection=server_.get_con_from_hdl(hdl);
	auto findResult=std::find_if( currentConnections_.begin(), currentConnections_.end(), [&pRawConnection](std::shared_ptr<communique::impl::Connection>& other){return pRawConnection==other->underlyingPointer();} );
	if( findResult!=currentConnections_.end() ) currentConnections_.erase( findResult );
	else std::cout << "Couldn't find connection to remove" << std::endl;

}

void communique::ServerPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has been interrupted" << std::endl;
//	auto findResult=std::find_if( currentConnections_.begin(), currentConnections_.end(), [&hdl](websocketpp::connection_hdl& other){return !other.owner_before(hdl) && !hdl.owner_before(other);} );
//	if( findResult!=currentConnections_.end() ) currentConnections_.erase( findResult );
}
