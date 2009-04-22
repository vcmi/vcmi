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

	class cachedSounds {
	public:
		std::string filename;
		Mix_Chunk *chunk;

		// This is some horrible C++ abuse. Isn't there any way to do
		// something simplier to init sounds?
		cachedSounds(std::string filename_in, Mix_Chunk *chunk_in):
		filename(filename_in), chunk(chunk_in) {};
	};

	std::map<soundBase::soundNames, cachedSounds> sounds;

	Mix_Chunk *GetSoundChunk(std::string srcName);

public:
	CMusicHandler(): sndh(NULL) {};
	void initMusics();

	// Sounds
	int playSound(soundBase::soundNames soundID); // plays sound wavs from Heroes3.snd
	int playSoundFromSet(std::vector<soundBase::soundNames> &sound_vec);

	// Sets
	std::vector<soundBase::soundNames> pickup_sounds;
};

#endif // __CMUSICHANDLER_H__
