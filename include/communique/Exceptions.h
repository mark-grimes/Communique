#ifndef communique_Exceptions_h
#define communique_Exceptions_h

#include <exception>

namespace communique
{

	/** @brief Base class for all exception thrown by Communique
	 *
	 * @author Mark Grimes (kknb1056@gmail.com)
	 * @date 30/Sep/2014
	 */
	class Exception : public std::exception
	{
	public:
		virtual ~Exception() {}
	};

} // end of namespace communique

#endif // end of ifndef communique_Exceptions_h
