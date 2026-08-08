/*
 * CAudioBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAudioBase.h"

#include <SDL3_mixer/SDL_mixer.h>

int CAudioBase::initializationCounter = 0;
bool CAudioBase::initializeSuccess = false;
MIX_Mixer * CAudioBase::mixer = nullptr;

CAudioBase::CAudioBase()
{
	if(initializationCounter == 0)
	{
		SDL_AudioSpec spec;
		spec.freq = 44100;
		spec.format = SDL_AUDIO_S16;
		spec.channels = 2;

		if(!MIX_Init())
			logGlobal->error("MIX_Init error: %s", SDL_GetError());
		else
		{
			mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);

			if(mixer == nullptr)
			{
				logGlobal->error("MIX_CreateMixerDevice error: %s", SDL_GetError());
				MIX_Quit();
			}
			else
				initializeSuccess = true;
		}
	}
	++initializationCounter;
}

bool CAudioBase::isInitialized() const
{
	return initializeSuccess;
}

MIX_Mixer * CAudioBase::getMixer()
{
	return mixer;
}

CAudioBase::~CAudioBase()
{
	--initializationCounter;

	if(initializationCounter == 0 && initializeSuccess)
	{
		MIX_DestroyMixer(mixer);
		mixer = nullptr;
		MIX_Quit();
		initializeSuccess = false;
	}
}
