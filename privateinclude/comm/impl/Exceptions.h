#ifndef comm_impl_Exceptions_h
#define comm_impl_Exceptions_h

#include <comm/Exceptions.h>
#include <string>

namespace comm
{

	namespace impl
	{
		/** @brief Class used when nothing else fits.
		 *
		 * @author Mark Grimes (kknb1056@gmail.com)
		 * @date 30/Sep/2014
		 */
		class Exception : public comm::Exception
		{
		public:
			Exception( const std::string& what );
			virtual ~Exception();
			virtual const char* what() const noexcept override;
		private:
			std::string what_;
		};

	} // end of namespace impl
} // end of namespace comm

#endif // end of ifndef comm_impl_Exceptions_h
