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

#include <test/tools/ossfuzz/SolidityGenerator.h>

#include <libsolutil/Whiskers.h>
#include <libsolutil/Visitor.h>

using namespace solidity::test::fuzzer::mutator;
using namespace solidity::util;

GeneratorBase::GeneratorBase(std::shared_ptr<SolidityGenerator> _mutator)
{
	mutator = std::move(_mutator);
	state = mutator->testState();
	uRandDist = mutator->uniformRandomDist();
}

std::string GeneratorBase::visitChildren()
{
	std::ostringstream os;
	// Randomise visit order
	std::vector<GeneratorPtr> randomisedChildren;
	for (auto const& child: generators)
		randomisedChildren.push_back(child);
	shuffle(randomisedChildren.begin(), randomisedChildren.end(), *uRandDist->randomEngine);
	for (auto child: randomisedChildren)
		os << std::visit(GenericVisitor{
			[&](auto const& _item) { return _item->generate(); }
		}, child);
	return os.str();
}

std::string TestState::randomPath(std::set<std::string> const& _sourceUnitPaths) const
{
	auto it = _sourceUnitPaths.begin();
	/// Advance iterator by n where 0 <= n <= sourceUnitPaths.size() - 1
	size_t increment = uRandDist->distributionOneToN(_sourceUnitPaths.size()) - 1;
	solAssert(
		increment >= 0 && increment < _sourceUnitPaths.size(),
		"Solc custom mutator: Invalid increment"
	);
	advance(it, increment);
	return *it;
}

std::string TestState::randomPath() const
{
	solAssert(!empty(), "Solc custom mutator: Null test state");
	return randomPath(sourceUnitPaths);
}

void TestState::print(std::ostream& _os) const
{
	_os << "Printing test state" << std::endl;
	for (auto const& item: sourceUnitPaths)
		_os << "Source path: " << item << std::endl;
}

std::string TestState::randomNonCurrentPath() const
{
	/// To obtain a source path that is not the currently visited
	/// source unit itself, we require at least one other source
	/// unit to be previously visited.
	solAssert(size() >= 2, "Solc custom mutator: Invalid test state");

	std::set<std::string> filteredSourcePaths;
	std::string currentPath = currentSourceUnitPath;
	std::copy_if(
		sourceUnitPaths.begin(),
		sourceUnitPaths.end(),
		inserter(filteredSourcePaths, filteredSourcePaths.begin()),
		[currentPath](std::string const& _item) {
			return _item != currentPath;
		}
	);
	return randomPath(filteredSourcePaths);
}

void TestCaseGenerator::setup()
{
	addGenerators({
		mutator->generator<SourceUnitGenerator>()
	});
}

std::string TestCaseGenerator::visit()
{
	std::ostringstream os;
	for (unsigned i = 0; i < uRandDist->distributionOneToN(s_maxSourceUnits); i++)
	{
		std::string sourcePath = path();
		os << "\n"
			<< "==== Source: "
			<< sourcePath
			<< " ===="
	        << "\n";
		updateSourcePath(sourcePath);
		m_numSourceUnits++;
		os << visitChildren();
	}
	return os.str();
}

void SourceUnitGenerator::setup()
{
	addGenerators({
		mutator->generator<ImportGenerator>(),
		mutator->generator<PragmaGenerator>()
	});
}

std::string SourceUnitGenerator::visit()
{
	return visitChildren();
}

std::string PragmaGenerator::visit()
{
	static constexpr const char* preamble = R"(
		pragma solidity >= 0.0.0;
		pragma experimental SMTChecker;
	)";
	// Choose equally at random from coder v1 and v2
	std::string abiPragma = "pragma abicoder v" +
		std::to_string(uRandDist->distributionOneToN(2)) +
		";\n";
	return preamble + abiPragma;
}

std::string ImportGenerator::visit()
{
	/*
	 * Case 1: No source units defined
	 * Case 2: One source unit defined
	 * Case 3: At least two source units defined
	 */
	std::ostringstream os;
	// Self import with a small probability only if
	// there is one source unit present in test.
	if (state->size() == 1)
	{
		if (uRandDist->probable(s_selfImportInvProb))
			os << "import "
			   << "\""
			   << state->randomPath()
			   << "\";";
	}
	else
	{
		// Import a different source unit if at least
		// two source units available.
		os << "import "
			<< "\""
			<< state->randomNonCurrentPath()
			<< "\";";
	}
	return os.str();
}

template <typename T>
std::shared_ptr<T> SolidityGenerator::generator()
{
	for (auto& g: m_generators)
		if (std::holds_alternative<std::shared_ptr<T>>(g))
			return std::get<std::shared_ptr<T>>(g);
	solAssert(false, "");
}

SolidityGenerator::SolidityGenerator(unsigned _seed)
{
	m_generators = {};
	m_urd = std::make_shared<UniformRandomDistribution>(std::make_unique<RandomEngine>(_seed));
	m_state = std::make_shared<TestState>(m_urd);
}

template <size_t I>
void SolidityGenerator::createGenerators()
{
	if constexpr (I < std::variant_size_v<Generator>)
	{
		createGenerator<std::variant_alternative_t<I, Generator>>();
		createGenerators<I + 1>();
	}
}

std::string SolidityGenerator::generateTestProgram()
{
	createGenerators();
	for (auto& g: m_generators)
		std::visit(GenericVisitor{
			[&](auto const& _item) { return _item->setup(); }
		}, g);
	std::string program = generator<TestCaseGenerator>()->generate();
	destroyGenerators();
	return program;
}
