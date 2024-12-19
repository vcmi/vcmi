/*
 * AssetGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class PlayerColor;
VCMI_LIB_NAMESPACE_END

class AssetGenerator
{
public:
	static void generateAll();
	static void createAdventureOptionsCleanBackground();
	static void createBigSpellBook();
	static void createPlayerColoredBackground(const PlayerColor & player);
	static void createCombatUnitNumberWindow();
	static void createCampaignBackground();
	static void createChroniclesCampaignImages();
	static void createPaletteShiftedSprites();
};
