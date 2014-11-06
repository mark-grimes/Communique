#include <communique/impl/UniqueTokenStorage.h>
#include "../catch.hpp"

#include <iostream>
#include <map>

SCENARIO( "Test that UniqueTokenStorage behaves as expected", "[UniqueTokenStorage]" )
{
	GIVEN( "A UniqueTokenStorage<uint64_t,uint32_t> instance" )
	{
		// Use a 32 bit unsigned int as the tokens, and a normal unsigned int for the values
		communique::impl::UniqueTokenStorage<uint64_t,uint8_t> myTokenStorage;

		WHEN( "I try to retrieve from an empty container" )
		{
			CHECK_THROWS( myTokenStorage.pop(9) );
			CHECK_THROWS( myTokenStorage.pop(0) );
			uint64_t result;
			CHECK( myTokenStorage.pop(0,result)==false );
			CHECK( myTokenStorage.pop(9,result)==false );
		}
		WHEN( "I try to retrieve previously stored items I get the correct ones back" )
		{
			std::vector< std::pair<uint64_t,uint8_t> > storedItems; // keep a record of what's stored so I can check
			// Fill with random values
			for( size_t index=0; index<200; ++index )
			{
				// Get a random number to store
				uint64_t newValue=std::rand()*static_cast<float>(std::numeric_limits<uint64_t>::max())/RAND_MAX;
				uint8_t newKey;
				REQUIRE_NOTHROW( newKey=myTokenStorage.push( newValue ) );
				storedItems.push_back( std::make_pair(newValue,newKey) );
			}

			// Try and retrieve each of these values and make sure the result is correct
			for( const auto& valueKeyPair : storedItems )
			{
				uint64_t retrievedValue;
				REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop( valueKeyPair.second ) );
				CHECK( retrievedValue==valueKeyPair.first );
			}
		}

		WHEN( "I try to completely fill a container" )
		{
			uint64_t newValue;
			uint8_t newKey;
			// Fill with arbitrary values, take out some as I go
			for( uint64_t index=0; index<=std::numeric_limits<uint8_t>::max(); ++index )
			{
				REQUIRE_NOTHROW( newKey=myTokenStorage.push( index ) );
			}

			newValue=34;
			CHECK_THROWS( newKey=myTokenStorage.push( newValue ) );

			// Try and retrieve an arbitrary entry
			uint8_t retrieveKey=64;
			uint64_t retrievedValue;
			REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop(retrieveKey) );
			// Then try and add another entry
			CHECK_NOTHROW( newKey=myTokenStorage.push( retrievedValue ) );
			// Collection should be full again, so make sure I can't add anything else
			CHECK_THROWS( newKey=myTokenStorage.push( newValue ) );
		}

		WHEN( "I fill then empty some items, I can still retrieve the correct ones back items" )
		{
			std::map<uint8_t,uint64_t> storedItems; // keep a record of what's stored so I can check
			uint64_t newValue;
			uint8_t newKey;
			// Fill with random values
			for( size_t index=0; index<=std::numeric_limits<uint8_t>::max(); ++index )
			{
				// Get a random number to store
				newValue=std::rand()*static_cast<float>(std::numeric_limits<uint64_t>::max())/RAND_MAX;
				REQUIRE_NOTHROW( newKey=myTokenStorage.push( newValue ) );
				storedItems[newKey]=newValue;
			}

			// Check container is actually full
			newValue=34;
			CHECK_THROWS( newKey=myTokenStorage.push( newValue ) );

			// Remove some arbitrary items
			uint64_t retrievedValue;
			REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop(0) );
			REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop(1) );
			REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop(2) );
			REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop(65) );
			REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop(128) );

			newValue=5;
			REQUIRE_NOTHROW( newKey=myTokenStorage.push( newValue ) );
			storedItems[newKey]=newValue;

			newValue=6;
			REQUIRE_NOTHROW( newKey=myTokenStorage.push( newValue ) );
			storedItems[newKey]=newValue;

			newValue=7;
			REQUIRE_NOTHROW( newKey=myTokenStorage.push( newValue ) );
			storedItems[newKey]=newValue;

			newValue=8;
			REQUIRE_NOTHROW( newKey=myTokenStorage.push( newValue ) );
			storedItems[newKey]=newValue;

			newValue=9;
			REQUIRE_NOTHROW( newKey=myTokenStorage.push( newValue ) );
			storedItems[newKey]=newValue;

			// Try and retrieve each of these values and make sure the result is correct
			for( const auto& keyValuePair : storedItems )
			{
				uint64_t retrievedValue;
				REQUIRE_NOTHROW( retrievedValue=myTokenStorage.pop( keyValuePair.first ) );
				CHECK( retrievedValue==keyValuePair.second );
			}
		}
	}
}
