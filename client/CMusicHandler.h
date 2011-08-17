#ifndef __CMUSICHANDLER_H__
#define __CMUSICHANDLER_H__

#include <boost/thread/mutex.hpp>

#include "CSoundBase.h"
#include "CMusicBase.h"
#include "../lib/CCreatureHandler.h"
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

class CSpell;
struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;
struct Mix_Chunk;


// Sound infos for creatures in combat
struct CreaturesBattleSounds {
	soundBase::soundID attack;
	soundBase::soundID defend;
	soundBase::soundID killed; // was killed or died
	soundBase::soundID move;
	soundBase::soundID shoot; // range attack
	soundBase::soundID wince; // attacked but did not die
	soundBase::soundID ext1;  // creature specific extension
	soundBase::soundID ext2;  // creature specific extension
	soundBase::soundID startMoving; // usually same as ext1
	soundBase::soundID endMoving;	// usually same as ext2

	CreaturesBattleSounds(): attack(soundBase::invalid),
							 defend(soundBase::invalid),
							 killed(soundBase::invalid),
							 move(soundBase::invalid),
							 shoot(soundBase::invalid),
							 wince(soundBase::invalid),
							 ext1(soundBase::invalid),
							 ext2(soundBase::invalid),
							 startMoving(soundBase::invalid),
							 endMoving(soundBase::invalid) {};
};

class CAudioBase {
protected:
	bool initialized;
	int volume;					// from 0 (mute) to 100

public:
	CAudioBase(): initialized(false), volume(0) {};
	virtual void init() = 0;
	virtual void release() = 0;

	virtual void setVolume(unsigned int percent);
	unsigned int getVolume() { return volume; };
};

class CSoundHandler: public CAudioBase
{
private:
	CSndHandler sndh;
	soundBase::soundID getSoundID(std::string &fileName);

	std::map<soundBase::soundID, Mix_Chunk *> soundChunks;

	Mix_Chunk *GetSoundChunk(soundBase::soundID soundID);

public:
	CSoundHandler();

	void init();
	void release();

	void initCreaturesSounds(const std::vector<ConstTransitivePtr<CCreature> > &creatures);
	void initSpellsSounds(const std::vector< ConstTransitivePtr<CSpell> > &spells);
	void setVolume(unsigned int percent);

	// Sounds
	int playSound(soundBase::soundID soundID, int repeats=0);
	int playSoundFromSet(std::vector<soundBase::soundID> &sound_vec);
	void stopSound(int handler);
	std::vector <struct CreaturesBattleSounds> CBattleSounds;
	std::map<const CSpell*, soundBase::soundID> spellSounds;

	// Sets
	std::vector<soundBase::soundID> pickupSounds;
	std::vector<soundBase::soundID> horseSounds;
};

// Helper
#define battle_sound(creature,what_sound) CCS->soundh->CBattleSounds[(creature)->idNumber].what_sound

class CMusicHandler: public CAudioBase
{
private:
	// Because we use the SDL music callback, our music variables must
	// be protected
	boost::mutex musicMutex;
	Mix_Music *currentMusic;
	Mix_Music *nextMusic;
	int nextMusicLoop;

	Mix_Music * LoadMUS(const char *file); //calls Mix_LoadMUS and checks for errors
	int PlayMusic(Mix_Music *music, int loops); //calls Mix_PlayMusic and checks for errors
public:
	CMusicHandler();

	void init();
	void release();
	void setVolume(unsigned int percent);

	// Musics
	std::map<musicBase::musicID, std::string> musics;
	std::vector<musicBase::musicID> battleMusics;
	std::vector<musicBase::musicID> townMusics;
	std::vector<musicBase::musicID> terrainMusics;

	void playMusic(musicBase::musicID musicID, int loop=1);
	void playMusicFromSet(std::vector<musicBase::musicID> &music_vec, int loop=1);
	void stopMusic(int fade_ms=1000);
	void musicFinishedCallback(void);
};

#endif // __CMUSICHANDLER_H__
