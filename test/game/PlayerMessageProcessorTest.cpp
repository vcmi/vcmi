/*
 * PlayerMessageProcessorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <gtest/gtest.h>
#include <fstream>
#include <regex>
#include <set>

// Test that validates cheat code syntax by parsing the actual implementation
// This catches issues like missing commas (which cause string concatenation)
// and ensures all declared cheats have corresponding implementations
TEST(PlayerMessageProcessorTest, CheatCodeVectorSyntaxValidation)
{
	// Try multiple possible paths to PlayerMessageProcessor.cpp
	std::vector<std::string> possiblePaths = {
		"server/processors/PlayerMessageProcessor.cpp",
		"../server/processors/PlayerMessageProcessor.cpp",
		"../../server/processors/PlayerMessageProcessor.cpp"
	};

	std::string filePath;
	std::ifstream file;

	for (const auto & path : possiblePaths)
	{
		file.open(path);
		if (file.is_open())
		{
			filePath = path;
			break;
		}
	}

	ASSERT_TRUE(file.is_open())
		<< "Could not find PlayerMessageProcessor.cpp in any of the expected locations";

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string fileContents = buffer.str();

	// Extract the cheat code vectors from the source
	// We look for patterns like: "cheatname",  or "cheatname"  at end of initializer
	std::regex cheatPattern(R"regex("([a-z0-9]+)")regex");

	// Find the localCheats, townTargetedCheats, playerTargetedCheats, and heroTargetedCheats vectors
	std::vector<std::string> allCheats;
	std::set<std::string> uniqueCheats;

	// Extract cheats from each vector declaration
	auto extractCheats = [&](const std::string & vectorName) -> std::vector<std::string>
	{
		size_t vectorPos = fileContents.find("std::vector<std::string> " + vectorName);
		if (vectorPos == std::string::npos)
			return {};

		// Find the closing brace of the vector initialization
		size_t openBrace = fileContents.find("{", vectorPos);
		size_t closeBrace = fileContents.find("};", openBrace);

		if (openBrace == std::string::npos || closeBrace == std::string::npos)
			return {};

		std::string vectorInit = fileContents.substr(openBrace, closeBrace - openBrace);

		// Extract all quoted strings (cheat codes)
		std::sregex_iterator iter(vectorInit.begin(), vectorInit.end(), cheatPattern);
		std::sregex_iterator end;

		std::vector<std::string> cheats;
		for (; iter != end; ++iter)
		{
			std::string cheat = (*iter)[1].str();
			cheats.push_back(cheat);
			allCheats.push_back(cheat);
			uniqueCheats.insert(cheat);
		}

		return cheats;
	};

	auto localCheats = extractCheats("localCheats");
	auto townTargetedCheats = extractCheats("townTargetedCheats");
	auto playerTargetedCheats = extractCheats("playerTargetedCheats");
	auto heroTargetedCheats = extractCheats("heroTargetedCheats");

	// Verify we found some cheats
	ASSERT_FALSE(allCheats.empty()) << "No cheat codes found in source file";
	ASSERT_GT(heroTargetedCheats.size(), 30)
		<< "Expected at least 30 hero-targeted cheats, found " << heroTargetedCheats.size();

	// KEY TEST 1: Verify no cheat code is unreasonably long (would indicate concatenation)
	for (const auto & cheat : allCheats)
	{
		EXPECT_LT(cheat.length(), 25)
			<< "Cheat code '" << cheat << "' is suspiciously long (" << cheat.length()
			<< " chars) - possible string concatenation bug";
	}

	// KEY TEST 2: Check for obvious concatenations by looking for multiple known cheat names in one string
	std::vector<std::string> knownCheats = {
		"vcmi", "nwc", "flesh", "wound", "glaurung", "machines", "skill"
	};

	for (const auto & cheat : allCheats)
	{
		int patternCount = 0;
		for (const auto & pattern : knownCheats)
		{
			size_t pos = 0;
			while ((pos = cheat.find(pattern, pos)) != std::string::npos)
			{
				patternCount++;
				pos += pattern.length();
			}
		}

		// If a cheat contains multiple distinct patterns, it's likely concatenated
		EXPECT_LE(patternCount, 2)
			<< "Cheat code '" << cheat << "' contains " << patternCount
			<< " distinct patterns - likely a concatenation bug";
	}

	// KEY TEST 3: Verify specific cheats that were affected by the bug
	EXPECT_TRUE(uniqueCheats.count("vcmiskill"))
		<< "vcmiskill should be recognized as a separate cheat code";

	EXPECT_TRUE(uniqueCheats.count("vcmimachines"))
		<< "vcmimachines should be recognized as a separate cheat code";

	EXPECT_TRUE(uniqueCheats.count("nwcfleshwound"))
		<< "nwcfleshwound should be a separate cheat code";

	EXPECT_TRUE(uniqueCheats.count("vcmiglaurung"))
		<< "vcmiglaurung should be a separate cheat code";

	// KEY TEST 4: Verify no accidental concatenation (the bug this test is designed to catch)
	EXPECT_FALSE(uniqueCheats.count("nwcfleshwoundvcmiglaurung"))
		<< "Found concatenated cheat 'nwcfleshwoundvcmiglaurung' - missing comma between cheats!";

	// KEY TEST 5: Extract callbacks map and verify all cheats have implementations
	size_t callbacksPos = fileContents.find("std::map<std::string, std::function<void()>> callbacks");
	ASSERT_NE(callbacksPos, std::string::npos) << "Could not find callbacks map in source file";

	size_t callbacksOpen = fileContents.find("{", callbacksPos);
	size_t callbacksClose = fileContents.find("};", callbacksOpen);
	ASSERT_NE(callbacksOpen, std::string::npos) << "Could not find callbacks map opening brace";
	ASSERT_NE(callbacksClose, std::string::npos) << "Could not find callbacks map closing brace";

	std::string callbacksSection = fileContents.substr(callbacksOpen, callbacksClose - callbacksOpen);

	// Extract callback keys - they look like: {"cheatname",
	std::regex callbackKeyPattern(R"regex(\{"([a-z0-9]+)")regex");
	std::set<std::string> implementedCheats;

	std::sregex_iterator callbackIter(callbacksSection.begin(), callbacksSection.end(), callbackKeyPattern);
	std::sregex_iterator callbackEnd;

	for (; callbackIter != callbackEnd; ++callbackIter)
	{
		implementedCheats.insert((*callbackIter)[1].str());
	}

	ASSERT_FALSE(implementedCheats.empty()) << "No callback implementations found";

	// Verify all declared cheats have implementations
	// Note: localCheats are excluded from callbacks map as they're handled differently
	std::vector<std::string> nonLocalCheats;
	for (const auto & cheat : townTargetedCheats)
		nonLocalCheats.push_back(cheat);
	for (const auto & cheat : playerTargetedCheats)
		nonLocalCheats.push_back(cheat);
	for (const auto & cheat : heroTargetedCheats)
		nonLocalCheats.push_back(cheat);

	for (const auto & cheat : nonLocalCheats)
	{
		EXPECT_TRUE(implementedCheats.count(cheat))
			<< "Cheat '" << cheat << "' is declared in a vector but has no callback implementation";
	}

	// Verify no orphaned implementations (cheats in callbacks but not in vectors)
	// This could happen if someone removes a cheat from vectors but forgets to remove the callback
	std::set<std::string> allDeclaredCheats(allCheats.begin(), allCheats.end());

	for (const auto & cheat : implementedCheats)
	{
		EXPECT_TRUE(allDeclaredCheats.count(cheat))
			<< "Cheat '" << cheat << "' has a callback implementation but is not declared in any vector";
	}

	// Summary information for debugging
	std::cout << "Validated " << allCheats.size() << " total cheat codes" << std::endl;
	std::cout << "  Local cheats: " << localCheats.size() << std::endl;
	std::cout << "  Town targeted: " << townTargetedCheats.size() << std::endl;
	std::cout << "  Player targeted: " << playerTargetedCheats.size() << std::endl;
	std::cout << "  Hero targeted: " << heroTargetedCheats.size() << std::endl;
	std::cout << "Found " << implementedCheats.size() << " callback implementations" << std::endl;
}
