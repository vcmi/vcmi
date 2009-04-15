#ifndef __CMUSICHANDLER_H__
#define __CMUSICHANDLER_H__

#include <SDL_mixer.h>
#include "CSndHandler.h"

/*
 * CMusicHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CMusicHandler
{
protected:
	CSndHandler *sndh;
public:
	Mix_Music *AITheme0, *AITheme1, *AITheme2, *combat1, *combat2, *combat3, *combat4, *castleTown, *defendCastle, *dirt, *dungeon, *elemTown, *evilTheme, *fortressTown, *goodTheme, *grass, *infernoTown, *lava, *loopLepr, *loseCampain, *loseCastle, *loseCombat, *mainMenu, *mainMenuWoG, *necroTown, *neutralTheme, *rampart, *retreatBattle, *rough, *sand, *secretTheme, *snow, *stronghold, *surrenderBattle, *swamp, *towerTown, *ultimateLose, *underground, *water, *winScenario, *winBattle;
	Mix_Chunk * buildTown, *click;
	void initMusics();
	void playClick(); //plays click music ;]
	void playLodSnd(std::string sndname);  // plays sound wavs from Heroes3.snd
};



#endif // __CMUSICHANDLER_H__
