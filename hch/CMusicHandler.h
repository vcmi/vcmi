#ifndef __CMUSICHANDLER_H__
#define __CMUSICHANDLER_H__

#include <boost/thread/mutex.hpp>

#include "CSoundBase.h"
#include "CMusicBase.h"


/*
 * CMusicHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CSndHandler;
struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;
struct Mix_Chunk;

class CSoundHandler
{
private:
	CSndHandler *sndh;
	soundBase::soundID getSoundID(std::string &fileName);

	std::map<soundBase::soundID, Mix_Chunk *> soundChunks;

	Mix_Chunk *GetSoundChunk(soundBase::soundID soundID);
	int volume;					// from 0 (mute) to 100

public:
	CSoundHandler(): sndh(NULL), volume(0) {};

	void initSounds();
	void freeSounds();
	void initCreaturesSounds(std::vector<CCreature> &creatures);

	// Sounds
	int playSound(soundBase::soundID soundID, int repeats=0);
	int playSoundFromSet(std::vector<soundBase::soundID> &sound_vec);
	void stopSound(int handler);
	void setSoundVolume(unsigned int percent);
	unsigned int getSoundVolume();

	// Sets
	std::vector<soundBase::soundID> pickupSounds;
	std::vector<soundBase::soundID> horseSounds;
};

class CMusicHandler
{
private:
	// Because we use the SDL music callback, our music variables must
	// be protected
	boost::mutex musicMutex;
	Mix_Music *currentMusic;
	Mix_Music *nextMusic;
	int nextMusicLoop;
	int volume;					// from 0 (mute) to 100

public:
	CMusicHandler(): currentMusic(NULL), nextMusic(NULL), volume(0) {};

	void initMusics();
	void freeMusics();

	// Musics
	std::map<musicBase::musicID, std::string> musics;
	std::vector<musicBase::musicID> battleMusics;

	void playMusic(musicBase::musicID musicID, int loop=1);
	void playMusicFromSet(std::vector<musicBase::musicID> &music_vec, int loop=1);
	void stopMusic(int fade_ms=1000);
	void setMusicVolume(unsigned int percent);
	unsigned int getMusicVolume();
	void musicFinishedCallback(void);
};

class CAudioHandler: public CSoundHandler, public CMusicHandler
{
private:
	bool audioInitialized;

public:
	CAudioHandler(): audioInitialized(false) {};
	~CAudioHandler();

	void initAudio(unsigned int volume = 90);
};

#endif // __CMUSICHANDLER_H__
