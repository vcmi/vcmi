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

#include <SDL_mixer.h>

int CAudioBase::initializationCounter = 0;
bool CAudioBase::initializeSuccess = false;

CAudioBase::CAudioBase()
{
	if(initializationCounter == 0)
	{
		if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
			logGlobal->error("Mix_OpenAudio error: %s", Mix_GetError());
		else
			initializeSuccess = true;
	}
	++initializationCounter;
}

bool CAudioBase::isInitialized() const
{
	return initializeSuccess;
}

CAudioBase::~CAudioBase()
{
	--initializationCounter;

	if(initializationCounter == 0 && initializeSuccess)
	{
		Mix_CloseAudio();
		initializeSuccess = false;
	}
}
