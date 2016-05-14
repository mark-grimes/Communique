/** @file Information required by all tests (locations of input files etcetera). */
#include <chrono>
#include <string>

struct testinputs
{
	// Port gets tied up after each test, so use a global and increment it
	// so that each test uses a different port
	static size_t portNumber;
	static const size_t browserTestPortNumber;
	static const std::chrono::milliseconds shortWait;
	static const std::string testFileDirectory;
};
