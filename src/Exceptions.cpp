#include <comm/impl/Exceptions.h>

comm::impl::Exception::Exception( const std::string& what ) : what_(what)
{

}

comm::impl::Exception::~Exception()
{

}

const char* comm::impl::Exception::what() const noexcept
{
	return what_.c_str();
}
