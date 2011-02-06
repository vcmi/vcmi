#ifndef __CMUSICBASE_H__
#define __CMUSICBASE_H__

/*
 * CMusicBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// Use some magic to keep the list of files and their code name in sync.

#define VCMI_MUSIC_LIST \
	VCMI_MUSIC_ID(AITheme0) VCMI_MUSIC_FILE("AITheme0.mp3") \
	VCMI_MUSIC_ID(AITheme1) VCMI_MUSIC_FILE("AITHEME1.MP3") \
	VCMI_MUSIC_ID(AITheme2) VCMI_MUSIC_FILE("AITHEME2.MP3") \
	VCMI_MUSIC_ID(bladeABCampaign) VCMI_MUSIC_FILE("BladeABCampaign.mp3") \
	VCMI_MUSIC_ID(bladeDBCampaign) VCMI_MUSIC_FILE("BladeDBCampaign.mp3") \
	VCMI_MUSIC_ID(bladeDSCampaign) VCMI_MUSIC_FILE("BladeDSCampaign.mp3") \
	VCMI_MUSIC_ID(bladeFLCampaign) VCMI_MUSIC_FILE("BladeFLCampaign.mp3") \
	VCMI_MUSIC_ID(bladeFWCampaign) VCMI_MUSIC_FILE("BladeFWCampaign.mp3") \
	VCMI_MUSIC_ID(bladePFCampaign) VCMI_MUSIC_FILE("BladePFCampaign.mp3") \
	VCMI_MUSIC_ID(campainMusic01) VCMI_MUSIC_FILE("CampainMusic01.mp3") \
	VCMI_MUSIC_ID(campainMusic02) VCMI_MUSIC_FILE("CampainMusic02.mp3") \
	VCMI_MUSIC_ID(campainMusic03) VCMI_MUSIC_FILE("CampainMusic03.mp3") \
	VCMI_MUSIC_ID(campainMusic04) VCMI_MUSIC_FILE("CampainMusic04.mp3") \
	VCMI_MUSIC_ID(campainMusic05) VCMI_MUSIC_FILE("CampainMusic05.mp3") \
	VCMI_MUSIC_ID(campainMusic06) VCMI_MUSIC_FILE("CampainMusic06.mp3") \
	VCMI_MUSIC_ID(campainMusic07) VCMI_MUSIC_FILE("CampainMusic07.mp3") \
	VCMI_MUSIC_ID(campainMusic08) VCMI_MUSIC_FILE("CampainMusic08.mp3") \
	VCMI_MUSIC_ID(campainMusic09) VCMI_MUSIC_FILE("CampainMusic09.mp3") \
	VCMI_MUSIC_ID(campainMusic10) VCMI_MUSIC_FILE("CampainMusic10.mp3") \
	VCMI_MUSIC_ID(campainMusic11) VCMI_MUSIC_FILE("CampainMusic11.mp3") \
	VCMI_MUSIC_ID(combat1) VCMI_MUSIC_FILE("COMBAT01.MP3") \
	VCMI_MUSIC_ID(combat2) VCMI_MUSIC_FILE("COMBAT02.MP3") \
	VCMI_MUSIC_ID(combat3) VCMI_MUSIC_FILE("COMBAT03.MP3") \
	VCMI_MUSIC_ID(combat4) VCMI_MUSIC_FILE("COMBAT04.MP3") \
	VCMI_MUSIC_ID(castleTown) VCMI_MUSIC_FILE("CstleTown.mp3") \
	VCMI_MUSIC_ID(defendCastle) VCMI_MUSIC_FILE("Defend Castle.mp3") \
	VCMI_MUSIC_ID(dirt) VCMI_MUSIC_FILE("DIRT.MP3") \
	VCMI_MUSIC_ID(dungeonTown) VCMI_MUSIC_FILE("DUNGEON.MP3") \
	VCMI_MUSIC_ID(elemTown) VCMI_MUSIC_FILE("ElemTown.mp3") \
	VCMI_MUSIC_ID(evilTheme) VCMI_MUSIC_FILE("EvilTheme.mp3") \
	VCMI_MUSIC_ID(fortressTown) VCMI_MUSIC_FILE("FortressTown.mp3") \
	VCMI_MUSIC_ID(goodTheme) VCMI_MUSIC_FILE("GoodTheme.mp3") \
	VCMI_MUSIC_ID(grass) VCMI_MUSIC_FILE("GRASS.MP3") \
	VCMI_MUSIC_ID(infernoTown) VCMI_MUSIC_FILE("InfernoTown.mp3") \
	VCMI_MUSIC_ID(lava) VCMI_MUSIC_FILE("LAVA.MP3") \
	VCMI_MUSIC_ID(loopLepr) VCMI_MUSIC_FILE("LoopLepr.mp3") \
	VCMI_MUSIC_ID(loseCampain) VCMI_MUSIC_FILE("Lose Campain.mp3") \
	VCMI_MUSIC_ID(loseCastle) VCMI_MUSIC_FILE("LoseCastle.mp3") \
	VCMI_MUSIC_ID(loseCombat) VCMI_MUSIC_FILE("LoseCombat.mp3") \
	VCMI_MUSIC_ID(mainMenu) VCMI_MUSIC_FILE("MAINMENU.MP3") \
	VCMI_MUSIC_ID(mainMenuWoG) VCMI_MUSIC_FILE("MainMenuWoG.mp3") \
	VCMI_MUSIC_ID(necroTown) VCMI_MUSIC_FILE("necroTown.mp3") \
	VCMI_MUSIC_ID(neutralTheme) VCMI_MUSIC_FILE("NeutralTheme.mp3") \
	VCMI_MUSIC_ID(rampartTown) VCMI_MUSIC_FILE("RAMPART.MP3") \
	VCMI_MUSIC_ID(retreatBattle) VCMI_MUSIC_FILE("Retreat Battle.mp3") \
	VCMI_MUSIC_ID(rough) VCMI_MUSIC_FILE("ROUGH.MP3") \
	VCMI_MUSIC_ID(sand) VCMI_MUSIC_FILE("SAND.MP3") \
	VCMI_MUSIC_ID(secretTheme) VCMI_MUSIC_FILE("SecretTheme.mp3") \
	VCMI_MUSIC_ID(snow) VCMI_MUSIC_FILE("SNOW.MP3") \
	VCMI_MUSIC_ID(strongHoldTown) VCMI_MUSIC_FILE("StrongHold.mp3") \
	VCMI_MUSIC_ID(surrenderBattle) VCMI_MUSIC_FILE("Surrender Battle.mp3") \
	VCMI_MUSIC_ID(swamp) VCMI_MUSIC_FILE("SWAMP.MP3") \
	VCMI_MUSIC_ID(towerTown) VCMI_MUSIC_FILE("TowerTown.mp3") \
	VCMI_MUSIC_ID(ultimateLose) VCMI_MUSIC_FILE("UltimateLose.mp3") \
	VCMI_MUSIC_ID(underground) VCMI_MUSIC_FILE("Underground.mp3") \
	VCMI_MUSIC_ID(water) VCMI_MUSIC_FILE("WATER.MP3") \
	VCMI_MUSIC_ID(winBattle) VCMI_MUSIC_FILE("Win Battle.mp3") \
	VCMI_MUSIC_ID(winScenario) VCMI_MUSIC_FILE("Win Scenario.mp3" )

class musicBase
{
public:
	// Make a list of enums
#define VCMI_MUSIC_ID(x) x,
#define VCMI_MUSIC_FILE(y)
	enum musicID {
		music_todo=0,			// temp entry until code is fixed
		VCMI_MUSIC_LIST
	};
#undef VCMI_MUSIC_ID
#undef VCMI_MUSIC_FILE
};

#endif // __CMUSICBASE_H__


