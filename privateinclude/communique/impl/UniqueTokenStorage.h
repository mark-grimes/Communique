#ifndef communique_impl_UniqueTokenStorage_h
#define communique_impl_UniqueTokenStorage_h

#include <list>
#include <mutex>
#include <iostream>

// Windows has some ridiculous macros defined that interfere with e.g. std::numeric_limits<int>::min().
// See for example http://stackoverflow.com/questions/5004858/stdmin-gives-error. Presumably non-windows
// systems won't have these defines and this code will be skipped implicitly.
#ifdef max
#	pragma push_macro("max")
#	undef max
#	define NEED_TO_RESET_MAX_MACRO
#endif
#ifdef min
#	pragma push_macro("min")
#	undef min
#	define NEED_TO_RESET_MIN_MACRO
#endif

namespace communique
{

	namespace impl
	{
		/** @brief A container that returns a unique token when an item is stored, which is used to retrieve it.
		 *
		 * When you store an object, the method returns a unique token. This token must be used
		 * to retrieve the object later. Tokens can be reused once the object has been retrieved.
		 *
		 * This is similar in concept to std::map, except that for a map you need to know the key
		 * before you can add an item. In this case you don't care what the key is when you insert.
		 *
		 * Thread safe through the use of locking.
		 *
		 * @author Mark Grimes (kknb1056@gmail.com)
		 * @date 24/Oct/2014
		 */
		template<class T_Element,class T_Token=uint32_t>
		class UniqueTokenStorage
		{
		public:
			UniqueTokenStorage();
			T_Token push( const T_Element& newElement );
			T_Token push( const T_Element&& newElement );
			/** @brief Remove and return the object associated to the token.
			 * Throws an exception if the token does not exist.*/
			T_Element pop( const T_Token& token );
			/** @brief Remove and move into the supplied object the object associated to the token.
			 * Returns false if the token doesn't exist.*/
			bool pop( const T_Token& token, T_Element& returnValue ) noexcept;
			/** @brief Return the object associated to the token without removing it.
			 * Throws an exception if the token does not exist.*/
			T_Element& at( const T_Token& token );
			const T_Element& at( const T_Token& token ) const;
			/** @brief Fills the supplied pointer with the address of the object.
			 * If the token doesn't exist returns false*/
			bool at( const T_Token& token, T_Element*& pReturnValue ) noexcept;
			bool at( const T_Token& token, const T_Element*& pReturnValue ) const noexcept;
		private:
			/// gets a token for both versions of store. N.B. assumes collection is already locked.
			std::pair<T_Token,typename std::list< std::pair<T_Token,T_Element> >::iterator> getFreeToken();
			std::list< std::pair<T_Token,T_Element> > container_;
			std::mutex lockMutex_;
		};

	} // end of namespace impl
} // end of namespace communique

template<class T_Element,class T_Token>
communique::impl::UniqueTokenStorage<T_Element,T_Token>::UniqueTokenStorage()
{

}

template<class T_Element,class T_Token>
T_Token communique::impl::UniqueTokenStorage<T_Element,T_Token>::push( const T_Element& newElement )
{
	std::lock_guard<std::mutex> guard(lockMutex_);

	// Get a free token and iterator telling me where to insert the new element
	auto tokenIteratorPair=getFreeToken();
	// Then insert the new element at the correct place in the collection...
	container_.emplace( tokenIteratorPair.second, tokenIteratorPair.first, newElement );
	// ...and return the token to the user.
	return tokenIteratorPair.first;
}

template<class T_Element,class T_Token>
T_Token communique::impl::UniqueTokenStorage<T_Element,T_Token>::push( const T_Element&& newElement )
{
	std::lock_guard<std::mutex> guard(lockMutex_);

	// Get a free token and iterator telling me where to insert the new element
	auto tokenIteratorPair=getFreeToken();
	// Then insert the new element at the correct place in the collection...
	container_.emplace( tokenIteratorPair.second, tokenIteratorPair.first, std::forward(newElement) );
	// ...and return the token to the user.
	return tokenIteratorPair.first;
}

template<class T_Element,class T_Token>
T_Element communique::impl::UniqueTokenStorage<T_Element,T_Token>::pop( const T_Token& token )
{
	//std::lock_guard<std::mutex> guard(lockMutex_);
	T_Element returnValue;
	if( !pop( token, returnValue ) ) throw std::runtime_error( "communique::impl::UniqueTokenStorage::pop couldn't find token" );
	return returnValue;
}

template<class T_Element,class T_Token>
bool communique::impl::UniqueTokenStorage<T_Element,T_Token>::pop( const T_Token& token, T_Element& returnValue ) noexcept
{
	std::lock_guard<std::mutex> guard(lockMutex_);

	auto iEntry=container_.begin();
	for( ; iEntry!=container_.end(); ++iEntry )
	{
		if( token==iEntry->first ) break;
	}

	if( iEntry==container_.end() ) return false; // Token not found

	returnValue=std::move( iEntry->second );
	container_.erase( iEntry );

	return true;
}

template<class T_Element,class T_Token>
T_Element& communique::impl::UniqueTokenStorage<T_Element,T_Token>::at( const T_Token& token )
{
	T_Element pReturnValue;
	if( !at( token, pReturnValue ) ) throw std::runtime_error( "communique::impl::UniqueTokenStorage::at couldn't find token" );
	return *pReturnValue;
}

template<class T_Element,class T_Token>
const T_Element& communique::impl::UniqueTokenStorage<T_Element,T_Token>::at( const T_Token& token ) const
{
	T_Element pReturnValue;
	if( !at( token, pReturnValue ) ) throw std::runtime_error( "communique::impl::UniqueTokenStorage::at couldn't find token" );
	return *pReturnValue;
}

template<class T_Element,class T_Token>
bool communique::impl::UniqueTokenStorage<T_Element,T_Token>::at( const T_Token& token, T_Element*& pReturnValue ) noexcept
{
	std::lock_guard<std::mutex> guard(lockMutex_);

	auto iEntry=container_.begin();
	for( ; iEntry!=container_.end(); ++iEntry )
	{
		if( token==iEntry->first ) break;
	}

	if( iEntry==container_.end() ) return false; // Token not found

	pReturnValue=&iEntry->second;

	return true;
}

template<class T_Element,class T_Token>
bool communique::impl::UniqueTokenStorage<T_Element,T_Token>::at( const T_Token& token, const T_Element*& pReturnValue ) const noexcept
{
	std::lock_guard<std::mutex> guard(lockMutex_);

	auto iEntry=container_.begin();
	for( ; iEntry!=container_.end(); ++iEntry )
	{
		if( token==iEntry->first ) break;
	}

	if( iEntry==container_.end() ) return false; // Token not found

	pReturnValue=&iEntry->second;

	return true;
}

template<class T_Element,class T_Token>
std::pair<T_Token,typename std::list< std::pair<T_Token,T_Element> >::iterator> communique::impl::UniqueTokenStorage<T_Element,T_Token>::getFreeToken()
{
	// Don't lock - assume this has already been done by the callee.

	// I need to find a token not already stored in the container_ list. Simplest case
	// is when the list is empty and I can use zero.
	T_Token token;
	if( container_.empty() ) return std::make_pair( std::numeric_limits<T_Token>::min(), container_.begin() );
	else
	{
		// Next simplest case is increasing the token in the last entry by one, assuming
		// it is not at its limit.
		token=container_.back().first;
		if( token!=std::numeric_limits<T_Token>::max() ) return std::make_pair( ++token, container_.end() );
		else
		{
			// The last entry can't be increased, so see if the first entry can't be decreased.
			token=container_.front().first;
			if( token!=std::numeric_limits<T_Token>::min() ) return std::make_pair( --token, container_.begin() );
			else
			{
				// Can't find any free space at the front or the back, so I need to run through
				// and find a free space somewhere in the middle.
				auto iEntry=container_.begin();
				for( ; iEntry!=container_.end(); ++iEntry, ++token )
				{
					if( token!=iEntry->first ) break;
				}
				// Make sure I was able to find free space
				if( iEntry==container_.end() ) throw std::runtime_error( "communique::impl::UniqueTokenStorage::push couldn't find free space" );
				return std::make_pair( token, iEntry );
			} // end of "else" for "if can't decrease the first token"
		} // end of "else" for "if can't increase the last token"
	} // end of "else" for "if empty"
}

// If this is windows, check if the macros undefined at the start need to be reset
#ifdef NEED_TO_RESET_MAX_MACRO
#	pragma pop_macro("max")
#	undef NEED_TO_RESET_MAX_MACRO
#endif
#ifdef NEED_TO_RESET_MIN_MACRO
#	pragma pop_macro("min")
#	undef NEED_TO_RESET_MIN_MACRO
#endif


#endif // end of ifndef communique_impl_UniqueTokenStorage_h
