#ifndef comm_Exceptions_h
#define comm_Exceptions_h

#include <exception>

namespace comm
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

} // end of namespace comm

#endif // end of ifndef comm_Exceptions_h
