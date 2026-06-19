/*
 * AIFactory.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class CGlobalAI;
class CBattleGameInterface;

namespace AIFactory
{
	DLL_LINKAGE std::shared_ptr<CGlobalAI> createAdventureAI(const std::string & name);
	DLL_LINKAGE std::shared_ptr<CBattleGameInterface> createBattleAI(const std::string & name);

	/// Returns true if the given name maps to a known statically-linked adventure AI.
	DLL_LINKAGE bool isAvailableAdventureAI(const std::string & name);
}

VCMI_LIB_NAMESPACE_END
