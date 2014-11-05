#include <communique/impl/Exceptions.h>

communique::impl::Exception::Exception( const std::string& what ) : what_(what)
{

}

communique::impl::Exception::~Exception()
{

}

const char* communique::impl::Exception::what() const noexcept
{
	return what_.c_str();
}
