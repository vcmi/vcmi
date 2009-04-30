#ifndef __CMUSICHANDLER_H__
#define __CMUSICHANDLER_H__

#include "CSoundBase.h"

/*
 * CMusicHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct Mix_Chunk;
class CSndHandler;

class CMusicHandler
{
private:
	CSndHandler *sndh;
	soundBase::soundID getSoundID(std::string &fileName);

	std::map<soundBase::soundID, Mix_Chunk *> soundChunks;

	Mix_Chunk *GetSoundChunk(soundBase::soundID soundID);

	bool audioInit;

public:
	CMusicHandler(): sndh(NULL), audioInit(false) {};
	~CMusicHandler();

	void initMusics();
	void initCreaturesSounds(std::vector<CCreature> &creatures);

	// Sounds
	int playSound(soundBase::soundID soundID, int repeats=0);
	int playSoundFromSet(std::vector<soundBase::soundID> &sound_vec);
	void stopSound(int handler);

	// Sets
	std::vector<soundBase::soundID> pickup_sounds;
	std::vector<soundBase::soundID> horseSounds;
};

#endif // __CMUSICHANDLER_H__
