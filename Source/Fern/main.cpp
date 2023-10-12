#include <catch.hpp>

int main(int argc, char** argv)
{
	//| Catch::Clara::Opt(bUnitTestRequired)["--test"]("Execute Unit Tests.");
	Catch::Session session;
	if (const int returnCode = session.applyCommandLine(argc, argv);
		returnCode != 0)
	{
		return returnCode;
	}

	const auto numOfFailedTestCase = session.run();
	return numOfFailedTestCase;
}