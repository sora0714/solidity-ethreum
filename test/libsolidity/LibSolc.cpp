/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * @date 2017
 * Unit tests for libsolc/libsolc.cpp.
 */

#include <string>
#include <boost/test/unit_test.hpp>
#include <libsolutil/JSON.h>
#include <libsolidity/interface/ReadFile.h>
#include <libsolidity/interface/Version.h>
#include <libsolc/libsolc.h>


namespace solidity::frontend::test
{

namespace
{

/// TODO: share this between StandardCompiler.cpp
/// Helper to match a specific error type and message
bool containsError(Json const& _compilerResult, std::string const& _type, std::string const& _message)
{
	if (!_compilerResult.contains("errors"))
		return false;

	for (auto const& error: _compilerResult["errors"])
	{
		BOOST_REQUIRE(error.is_object());
		BOOST_REQUIRE(error["type"].is_string());
		BOOST_REQUIRE(error["message"].is_string());
		if ((error["type"].get<std::string>() == _type) && (error["message"].get<std::string>() == _message))
			return true;
	}

	return false;
}

Json compile(std::string const& _input, CStyleReadFileCallback _callback = nullptr)
{
	char* output_ptr = solidity_compile(_input.c_str(), _callback, nullptr);
	std::string output(output_ptr);
	solidity_free(output_ptr);
	solidity_reset();
	Json ret;
	BOOST_REQUIRE(util::jsonParseStrict(output, ret));
	return ret;
}

char* stringToSolidity(std::string const& _input)
{
	char* ptr = solidity_alloc(_input.length());
	BOOST_REQUIRE(ptr != nullptr);
	std::memcpy(ptr, _input.c_str(), _input.length());
	return ptr;
}

} // end anonymous namespace

BOOST_AUTO_TEST_SUITE(LibSolc)

BOOST_AUTO_TEST_CASE(read_version)
{
	std::string output(solidity_version());
	BOOST_CHECK(output.find(VersionString) == 0);
}

BOOST_AUTO_TEST_CASE(read_license)
{
	std::string output(solidity_license());
	BOOST_CHECK(output.find("GNU GENERAL PUBLIC LICENSE") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(standard_compilation)
{
	char const* input = R"(
	{
		"language": "Solidity",
		"sources": {
			"fileA": {
				"content": "contract A { }"
			}
		}
	}
	)";
	Json result = compile(input);
	BOOST_REQUIRE(result.is_object());

	// Only tests some assumptions. The StandardCompiler is tested properly in another suite.
	BOOST_CHECK(result.contains("sources"));
	// This used to test that it is a member, but we did not actually request any output,
	// so there should not be a contract member.
	BOOST_CHECK(!result.contains("contracts"));
}

BOOST_AUTO_TEST_CASE(missing_callback)
{
	char const* input = R"(
	{
		"language": "Solidity",
		"sources": {
			"fileA": {
				"content": "import \"missing.sol\"; contract A { }"
			}
		}
	}
	)";
	Json result = compile(input);
	BOOST_REQUIRE(result.is_object());

	BOOST_CHECK(containsError(result, "ParserError", "Source \"missing.sol\" not found: File not supplied initially."));
}

BOOST_AUTO_TEST_CASE(with_callback)
{
	char const* input = R"(
	{
		"language": "Solidity",
		"sources": {
			"fileA": {
				"content": "import \"found.sol\"; import \"notfound.sol\"; contract A { }"
			}
		}
	}
	)";

	CStyleReadFileCallback callback{
		[](void* _context, char const* _kind, char const* _path, char** o_contents, char** o_error)
		{
			// Passed in a nullptr in the compile() helper above.
			BOOST_REQUIRE(_context == nullptr);
			// Caller frees the pointers.
			BOOST_REQUIRE(std::string(_kind) == ReadCallback::kindString(ReadCallback::Kind::ReadFile));
			if (std::string(_path) == "found.sol")
			{
				static std::string content{"import \"missing.sol\"; contract B {}"};
				*o_contents = stringToSolidity(content);
				*o_error = nullptr;
			}
			else if (std::string(_path) == "missing.sol")
			{
				static std::string errorMsg{"Missing file."};
				*o_error = stringToSolidity(errorMsg);
				*o_contents = nullptr;
			}
			else
			{
				*o_error = nullptr;
				*o_contents = nullptr;
			}
		}
	};

	Json result = compile(input, callback);
	BOOST_REQUIRE(result.is_object());

	// This ensures that "found.sol" was properly loaded which triggered the second import statement.
	BOOST_CHECK(containsError(result, "ParserError", "Source \"missing.sol\" not found: Missing file."));

	// This should be placed due to the missing "notfound.sol" which sets both pointers to null.
	BOOST_CHECK(containsError(result, "ParserError", "Source \"notfound.sol\" not found: Callback not supported."));
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespaces
