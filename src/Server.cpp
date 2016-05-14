#include "communique/Server.h"

#define _WEBSOCKETPP_CPP11_STL_ // Make sure websocketpp uses c++11 features in preference to boost ones
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio.hpp>
#include <list>
#include <fstream>
#include "communique/impl/Connection.h"
#include "communique/impl/TLSHandler.h"


//
// Declaration of the pimple
//
namespace communique
{
	class ServerPrivateMembers
	{
	public:
		typedef websocketpp::server<websocketpp::config::asio_tls> server_type;

		ServerPrivateMembers() : tlsHandler_(server_.get_alog(),true) { /*No operation besides initialiser list*/ }
		server_type server_;
		std::thread ioThread_;
		std::list< std::shared_ptr<communique::impl::Connection> > currentConnections_;
		mutable std::mutex currentConnectionsMutex_;
		communique::impl::TLSHandler tlsHandler_;

		//
		// These functions define the different behaviour when a HTTP connection is made instead
		// of a websocket one. Only one of them will be active at a time. The options are:
		//    ignore                - completely ignore the request and don't respond
		//    acknowledge (default) - just acknowledge the request with a HTTP "ok" response (regardless
		//                            of the resource). Useful for forcing certificate selection when
		//                            connecting from a browser.
		//    simpleFileServer      - serve a file with the filename of the resource request.
		//
		void on_http_ignore( websocketpp::connection_hdl hdl );
		void on_http_acknowledge( websocketpp::connection_hdl hdl );
		void on_http_simpleFileServer( websocketpp::connection_hdl hdl );
		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );

		std::string simpleFileServerRoot;
		std::function<void(const std::string&,std::weak_ptr<communique::IConnection>)> defaultInfoHandler_;
		std::function<std::string(const std::string&,std::weak_ptr<communique::IConnection>)> defaultRequestHandler_;
	};
}

communique::Server::Server()
	: pImple_( new ServerPrivateMembers )
{
	pImple_->server_.set_access_channels(websocketpp::log::alevel::none);
	//pImple_->server_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	pImple_->server_.set_error_channels(websocketpp::log::elevel::none);
	pImple_->server_.set_tls_init_handler( std::bind( &communique::impl::TLSHandler::on_tls_init, &pImple_->tlsHandler_, std::placeholders::_1 ) );
	pImple_->server_.set_http_handler( std::bind( &ServerPrivateMembers::on_http_acknowledge, pImple_.get(), std::placeholders::_1 ) );
	pImple_->server_.init_asio();
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

		websocketpp::lib::error_code errorCode;
		websocketpp::lib::asio::error_code underlyingErrorCode;
		pImple_->server_.listen(port,errorCode,&underlyingErrorCode);
		if( errorCode )
		{
			std::string errorMessage="Communique server listen error: "+errorCode.message();
			if( underlyingErrorCode ) errorMessage+=" ("+underlyingErrorCode.message()+")";
			throw std::runtime_error( errorMessage );
		}

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
	pImple_->tlsHandler_.setCertificateChainFile(filename);
}

void communique::Server::setPrivateKeyFile( const std::string& filename )
{
	pImple_->tlsHandler_.setPrivateKeyFile(filename);
}

void communique::Server::setVerifyFile( const std::string& filename )
{
	pImple_->tlsHandler_.setVerifyFile(filename);
}

void communique::Server::setDiffieHellmanParamsFile( const std::string& filename )
{
	pImple_->tlsHandler_.setDiffieHellmanParamsFile(filename);
}

void communique::Server::setDefaultInfoHandler( std::function<void(const std::string&)> infoHandler )
{
	// Wrap in a function that drops the connection argument
	pImple_->defaultInfoHandler_=std::bind( infoHandler, std::placeholders::_1 );
}

void communique::Server::setDefaultInfoHandler( std::function<void(const std::string&,std::weak_ptr<communique::IConnection>)> infoHandler )
{
	pImple_->defaultInfoHandler_=infoHandler;
}

void communique::Server::setDefaultRequestHandler( std::function<std::string(const std::string&)> requestHandler )
{
	// Wrap in a function that drops the connection argument
	pImple_->defaultRequestHandler_=std::bind( requestHandler, std::placeholders::_1 );
}

void communique::Server::setDefaultRequestHandler( std::function<std::string(const std::string&,std::weak_ptr<communique::IConnection>)> requestHandler )
{
	pImple_->defaultRequestHandler_=requestHandler;
}

void communique::Server::setHTTPBehaviour( HTTPBehaviour behaviour )
{
	switch( behaviour )
	{
	case HTTPBehaviour::ignore :
		pImple_->server_.set_http_handler( std::bind( &ServerPrivateMembers::on_http_ignore, pImple_.get(), std::placeholders::_1 ) );
		pImple_->server_.get_alog().write( websocketpp::log::alevel::http, "Configured to ignore HTTP requests" );
		return;
	case HTTPBehaviour::acknowledge :
		pImple_->server_.set_http_handler( std::bind( &ServerPrivateMembers::on_http_acknowledge, pImple_.get(), std::placeholders::_1 ) );
		pImple_->server_.get_alog().write( websocketpp::log::alevel::http, "Configured to acknowledge HTTP requests" );
		return;
	case HTTPBehaviour::file_serve :
		pImple_->server_.set_http_handler( std::bind( &ServerPrivateMembers::on_http_simpleFileServer, pImple_.get(), std::placeholders::_1 ) );
		pImple_->server_.get_alog().write( websocketpp::log::alevel::http, "Configured to serve files for HTTP requests" );
		return;
	default :
		throw std::runtime_error( "Unknown behaviour requested for HTTP requests" );
	}
}

void communique::Server::setFileServeRoot( const std::string& rootDirectory )
{
	pImple_->server_.set_http_handler( std::bind( &ServerPrivateMembers::on_http_simpleFileServer, pImple_.get(), std::placeholders::_1 ) );
	pImple_->simpleFileServerRoot=rootDirectory;
	pImple_->server_.get_alog().write( websocketpp::log::alevel::http, "Configured to serve files from "+rootDirectory );
}

std::vector<std::weak_ptr<communique::IConnection> > communique::Server::currentConnections()
{
	std::lock_guard<std::mutex> myMutex( pImple_->currentConnectionsMutex_ );
	return std::vector<std::weak_ptr<communique::IConnection> >( pImple_->currentConnections_.begin(), pImple_->currentConnections_.end() );
}

void communique::Server::setErrorLogLocation( std::ostream& outputStream )
{
	pImple_->server_.get_elog().set_ostream( &outputStream );
}

void communique::Server::setErrorLogLevel( uint32_t level )
{
	pImple_->server_.set_error_channels(level);
}

void communique::Server::setAccessLogLocation( std::ostream& outputStream )
{
	pImple_->server_.get_alog().set_ostream( &outputStream );
}

void communique::Server::setAccessLogLevel( uint32_t level )
{
	pImple_->server_.set_access_channels(level);
}

void communique::ServerPrivateMembers::on_http_ignore( websocketpp::connection_hdl hdl )
{
}

void communique::ServerPrivateMembers::on_http_acknowledge( websocketpp::connection_hdl hdl )
{
	server_type::connection_ptr con = server_.get_con_from_hdl(hdl);
	//con->set_body("Hello World!\n");
	con->set_status(websocketpp::http::status_code::ok);
}

void communique::ServerPrivateMembers::on_http_simpleFileServer( websocketpp::connection_hdl hdl )
{
	server_type::connection_ptr pConnection = server_.get_con_from_hdl(hdl);
	if( pConnection )
	{
		std::ifstream file( simpleFileServerRoot+pConnection->get_resource(), std::iostream::binary );
		if( file.is_open() )
		{
			std::noskipws(file);
			std::string fileContents;
			file.seekg(0, std::ios::end); // Jump to the end to figure out the size of the file
			const int fileSize=file.tellg();
			file.seekg(0, std::ios::beg); // Jump back to the start before reading
			if( fileSize>0 )
			{
				fileContents.reserve( fileSize ); // Reserve the correct size for the string to store the whole file
				fileContents.assign( (std::istream_iterator<char>(file)), std::istream_iterator<char>() );
				pConnection->set_body( fileContents );
				pConnection->set_status( websocketpp::http::status_code::ok );
			}
			else pConnection->set_status( websocketpp::http::status_code::internal_server_error );
		}
		else pConnection->set_status( websocketpp::http::status_code::not_found );
	}
}

void communique::ServerPrivateMembers::on_open( websocketpp::connection_hdl hdl )
{
	std::lock_guard<std::mutex> myMutex( currentConnectionsMutex_ );
	currentConnections_.emplace_back( new communique::impl::Connection( server_.get_con_from_hdl(hdl), defaultInfoHandler_, defaultRequestHandler_ ) );
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
