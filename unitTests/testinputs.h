/** @file Information required by all tests. */

namespace testinputs
{
	// Port gets tied up after each test, so use a global and increment it
	// so that each test uses a different port
	static size_t portNumber=9108;
	static const auto shortWait=std::chrono::milliseconds(50);
	static const std::string testFileDirectory="unitTests/testData/";
}
