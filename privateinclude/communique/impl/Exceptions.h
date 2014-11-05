#ifndef communique_impl_Exceptions_h
#define communique_impl_Exceptions_h

#include <communique/Exceptions.h>
#include <string>

namespace communique
{

	namespace impl
	{
		/** @brief Class used when nothing else fits.
		 *
		 * @author Mark Grimes (kknb1056@gmail.com)
		 * @date 30/Sep/2014
		 */
		class Exception : public communique::Exception
		{
		public:
			Exception( const std::string& what );
			virtual ~Exception();
			virtual const char* what() const noexcept override;
		private:
			std::string what_;
		};

	} // end of namespace impl
} // end of namespace communique

#endif // end of ifndef communique_impl_Exceptions_h
