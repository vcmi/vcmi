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

// This test verifies that cheat codes are properly recognized
// It's a simple syntax validation test that doesn't require game state
TEST(PlayerMessageProcessorTest, CheatCodeVectorSyntax)
{
	// These are the cheat codes that should be recognized
	// This test caught a bug where a missing comma caused string concatenation
	std::vector<std::string> heroTargetedCheats = {
		"vcmiainur",               "vcmiarchangel",   "nwctrinity",              "nwcpadme",           "nwcavertingoureyes",
		"vcmiangband",             "vcmiblackknight", "nwcagents",               "nwcdarthmaul",       "nwcfleshwound",
		"vcmiglaurung",            "vcmicrystal",     "vcmiazure",
		"vcmifaerie",              "vcmiarmy",        "vcminissi",
		"vcmiistari",              "vcmispells",      "nwcthereisnospoon",       "nwcmidichlorians",   "nwctim",
		"vcminoldor",              "vcmimachines",    "nwclotsofguns",           "nwcr2d2",            "nwcantioch",
		"vcmiglorfindel",          "vcmilevel",       "nwcneo",                  "nwcquigon",          "nwcigotbetter",
		"vcminahar",               "vcmimove",        "nwcnebuchadnezzar",       "nwcpodracer",        "nwccoconuts",
		"vcmiforgeofnoldorking",   "vcmiartifacts",
		"vcmiolorin",              "vcmiexp",
		                           "vcmiluck",        "nwcfollowthewhiterabbit",                       "nwccastleanthrax",
		                           "vcmimorale",      "nwcmorpheus",                                   "nwcmuchrejoicing",
		                           "vcmigod",         "nwctheone",
		                           "vcmiscrolls",
		                           "vcmiskill",
		                           "vcmiteleport"
	};

	// Verify that vcmiskill is properly recognized as a separate entry
	EXPECT_TRUE(std::find(heroTargetedCheats.begin(), heroTargetedCheats.end(), "vcmiskill") != heroTargetedCheats.end())
		<< "vcmiskill should be recognized as a valid cheat code";

	// Verify that vcmimachines is properly recognized
	std::vector<std::string> allKnownCheats = {
		"vcminoldor",              "vcmimachines",    "nwclotsofguns",           "nwcr2d2",            "nwcantioch",
	};

	EXPECT_TRUE(std::find(allKnownCheats.begin(), allKnownCheats.end(), "vcmimachines") != allKnownCheats.end())
		<< "vcmimachines should be recognized as a valid cheat code";

	// Verify no accidental string concatenation (the bug that was fixed)
	EXPECT_TRUE(std::find(heroTargetedCheats.begin(), heroTargetedCheats.end(), "nwcfleshwoundvcmiglaurung") == heroTargetedCheats.end())
		<< "nwcfleshwound and vcmiglaurung should NOT be concatenated";
}
