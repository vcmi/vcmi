#include "StdInc.h"
#include <boost/bimap.hpp>
#include <SDL_mixer.h>

#include "CSndHandler.h"
#include "CMusicHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CSpellHandler.h"
#include "../client/CGameInfo.h"
#include "../lib/JsonNode.h"
#include "../lib/GameConstants.h"
#include "../lib/Filesystem/CResourceLoader.h"

/*
 * CMusicHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

using namespace boost::assign;

static boost::bimap<soundBase::soundID, std::string> sounds;

// Not pretty, but there's only one music handler object in the game.
static void soundFinishedCallbackC(int channel)
{
	CCS->soundh->soundFinishedCallback(channel);
}

static void musicFinishedCallbackC(void)
{
	CCS->musich->musicFinishedCallback();
}

void CAudioBase::init()
{
	if (initialized)
		return;

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1)
	{
		tlog1 << "Mix_OpenAudio error: " << Mix_GetError() << std::endl;
		return;
	}

	initialized = true;
}

void CAudioBase::release()
{
	if (initialized)
	{
		Mix_CloseAudio();
		initialized = false;
	}
}

void CAudioBase::setVolume(ui32 percent)
{
	if (percent > 100)
		percent = 100;

	volume = percent;
}

void CSoundHandler::onVolumeChange(const JsonNode &volumeNode)
{
	setVolume(volumeNode.Float());
}

CSoundHandler::CSoundHandler():
	listener(settings.listen["general"]["sound"])
{
	listener(boost::bind(&CSoundHandler::onVolumeChange, this, _1));
	// Map sound names
#define VCMI_SOUND_NAME(x) ( soundBase::x,
#define VCMI_SOUND_FILE(y) #y )
	sounds = boost::assign::list_of<boost::bimap<soundBase::soundID, std::string>::relation>
		VCMI_SOUND_LIST;
#undef VCMI_SOUND_NAME
#undef VCMI_SOUND_FILE

	// Vectors for helper(s)
	pickupSounds += soundBase::pickup01, soundBase::pickup02, soundBase::pickup03,
		soundBase::pickup04, soundBase::pickup05, soundBase::pickup06, soundBase::pickup07;

	horseSounds +=  // must be the same order as terrains (see EterrainType);
		soundBase::horseDirt, soundBase::horseSand, soundBase::horseGrass,
		soundBase::horseSnow, soundBase::horseSwamp, soundBase::horseRough,
		soundBase::horseSubterranean, soundBase::horseLava,
		soundBase::horseWater, soundBase::horseRock;

	battleIntroSounds +=     soundBase::battle00, soundBase::battle01,
	    soundBase::battle02, soundBase::battle03, soundBase::battle04,
	    soundBase::battle05, soundBase::battle06, soundBase::battle07;
};

void CSoundHandler::init()
{
	CAudioBase::init();

	if (initialized)
	{
		// Load sounds
		Mix_ChannelFinished(soundFinishedCallbackC);
	}
}

void CSoundHandler::release()
{
	if (initialized)
	{
		Mix_HaltChannel(-1);

		std::map<soundBase::soundID, Mix_Chunk *>::iterator it;
		for (it=soundChunks.begin(); it != soundChunks.end(); it++)
		{
			if (it->second)
				Mix_FreeChunk(it->second);
		}
	}

	CAudioBase::release();
}

// Allocate an SDL chunk and cache it.
Mix_Chunk *CSoundHandler::GetSoundChunk(soundBase::soundID soundID)
{
	// Find its name
	boost::bimap<soundBase::soundID, std::string>::left_iterator it;
	it = sounds.left.find(soundID);
	if (it == sounds.left.end())
		return nullptr;

	// Load and insert
	try
	{
		auto data = CResourceHandler::get()->loadData(ResourceID(std::string("SOUNDS/") + it->second, EResType::SOUND));

		SDL_RWops *ops = SDL_RWFromMem(data.first.release(), data.second);
		Mix_Chunk *chunk;
		chunk = Mix_LoadWAV_RW(ops, 1);	// will free ops
		soundChunks.insert(std::pair<soundBase::soundID, Mix_Chunk *>(soundID, chunk));
		return chunk;
	}
	catch(std::exception &e)
	{
		tlog3 << "Cannot get sound " << soundID << " chunk: " << e.what() << "\n";
		return nullptr;
	}
}

// Get a soundID given a filename
soundBase::soundID CSoundHandler::getSoundID(const std::string &fileName)
{
	boost::bimap<soundBase::soundID, std::string>::right_iterator it;

	it = sounds.right.find(fileName);
	if (it == sounds.right.end())
		return soundBase::invalid;
	else
		return it->second;
}

void CSoundHandler::initCreaturesSounds(const std::vector<ConstTransitivePtr< CCreature> > &creatures)
{
	tlog5 << "\t\tReading config/cr_sounds.json" << std::endl;
	const JsonNode config(ResourceID("config/cr_sounds.json"));

	CBattleSounds.resize(creatures.size());

	if (!config["creature_sounds"].isNull()) {

		BOOST_FOREACH(const JsonNode &node, config["creature_sounds"].Vector()) {
			const JsonNode *value;
			int id;

			value = &node["name"];

			bmap<std::string,int>::const_iterator i = CGI->creh->nameToID.find(value->String());
			if (i != CGI->creh->nameToID.end())
				id = i->second;
			else
			{
				tlog1 << "Sound info for an unknown creature: " << value->String() << std::endl;
				continue;
			}

			/* This is a bit ugly. Maybe we should use an array for
			 * sound ids instead of separate variables and define
			 * attack/defend/killed/... as indexes. */
#define GET_SOUND_VALUE(value_name) do { value = &node[#value_name]; if (!value->isNull()) CBattleSounds[id].value_name = getSoundID(value->String()); } while(0)

			GET_SOUND_VALUE(attack);
			GET_SOUND_VALUE(defend);
			GET_SOUND_VALUE(killed);
			GET_SOUND_VALUE(move);
			GET_SOUND_VALUE(shoot);
			GET_SOUND_VALUE(wince);
			GET_SOUND_VALUE(ext1);
			GET_SOUND_VALUE(ext2);
			GET_SOUND_VALUE(startMoving);
			GET_SOUND_VALUE(endMoving);
#undef GET_SOUND_VALUE
		}
	}

	//commented to avoid spurious warnings
	/*
	// Find creatures without sounds
	for(ui32 i=0;i<creatures.size();i++)
	{
		// Note: this will exclude war machines, but it's better
		// than nothing.
		if (vstd::contains(CGI->creh->notUsedMonsters, i))
			continue;

		CCreature &c = creatures[i];
		if (c.sounds.killed == soundBase::invalid)
			tlog1 << "creature " << c.idNumber << " doesn't have sounds" << std::endl;
	}*/
}

void CSoundHandler::initSpellsSounds(const std::vector< ConstTransitivePtr<CSpell> > &spells)
{
	const JsonNode config(ResourceID("config/sp_sounds.json"));

	if (!config["spell_sounds"].isNull()) {
		BOOST_FOREACH(const JsonNode &node, config["spell_sounds"].Vector()) {
			int spellid = node["id"].Float();
			const CSpell *s = CGI->spellh->spells[spellid];

			if (vstd::contains(spellSounds, s))
				tlog1 << "Spell << " << spellid << " already has a sound" << std::endl;

			soundBase::soundID sound = getSoundID(node["soundfile"].String());
			if (sound == soundBase::invalid)
				tlog0 << "Error: invalid sound for id "<< spellid << "\n";
			spellSounds[s] = sound;
		}
	}
}

// Plays a sound, and return its channel so we can fade it out later
int CSoundHandler::playSound(soundBase::soundID soundID, int repeats)
{
	if (!initialized)
		return -1;

	int channel;
	Mix_Chunk *chunk = GetSoundChunk(soundID);

	if (chunk)
	{
		channel = Mix_PlayChannel(-1, chunk, repeats);
		if (channel == -1)
			tlog1 << "Unable to play sound file " << soundID << " , error " << Mix_GetError() << std::endl;
		else
			callbacks[channel];//insert empty callback
	}
	else
	{
		channel = -1;
	}

	return channel;
}

// Helper. Randomly select a sound from an array and play it
int CSoundHandler::playSoundFromSet(std::vector<soundBase::soundID> &sound_vec)
{
	return playSound(sound_vec[rand() % sound_vec.size()]);
}

void CSoundHandler::stopSound( int handler )
{
	if (initialized && handler != -1)
		Mix_HaltChannel(handler);
}

// Sets the sound volume, from 0 (mute) to 100
void CSoundHandler::setVolume(ui32 percent)
{
	CAudioBase::setVolume(percent);

	if (initialized)
		Mix_Volume(-1, (MIX_MAX_VOLUME * volume)/100);
}

void CSoundHandler::setCallback(int channel, boost::function<void()> function)
{
	std::map<int, boost::function<void()> >::iterator iter;
	iter = callbacks.find(channel);

	//channel not found. It may have finished so fire callback now
	if(iter == callbacks.end())
		function();
	else
		iter->second = function;
}

void CSoundHandler::soundFinishedCallback(int channel)
{
	std::map<int, boost::function<void()> >::iterator iter;
	iter = callbacks.find(channel);

	assert(iter != callbacks.end());

	if (iter->second)
		iter->second();

	callbacks.erase(iter);
}

void CMusicHandler::onVolumeChange(const JsonNode &volumeNode)
{
	setVolume(volumeNode.Float());
}

CMusicHandler::CMusicHandler():
	listener(settings.listen["general"]["music"])
{
	listener(boost::bind(&CMusicHandler::onVolumeChange, this, _1));
	// Map music IDs
	// Vectors for helper
	const std::string setEnemy[] = {"AITheme0", "AITheme1", "AITheme2"};
	const std::string setBattle[] = {"Combat01", "Combat02", "Combat03", "Combat04"};
	const std::string setTerrain[] = {"Dirt",	"Sand",	"Grass", "Snow", "Swamp", "Rough", "Underground", "Lava", "Water"};
	const std::string setTowns[] =  {"CstleTown", "Rampart", "TowerTown", "InfernoTown",
	        "NecroTown", "Dungeon", "Stronghold", "FortressTown", "ElemTown"};

	auto fillSet = [=](std::string setName, const std::string list[], size_t amount)
	{
		for (size_t i=0; i < amount; i++)
	        addEntryToSet(setName, i, std::string("music/") + list[i]);
	};
	fillSet("enemy-turn", setEnemy, ARRAY_COUNT(setEnemy));
	fillSet("battle", setBattle, ARRAY_COUNT(setBattle));
	fillSet("terrain", setTerrain, ARRAY_COUNT(setTerrain));
	fillSet("town-theme", setTowns, ARRAY_COUNT(setTowns));
}

void CMusicHandler::addEntryToSet(std::string set, int musicID, std::string musicURI)
{
	musicsSet[set][musicID] = musicURI;
}

void CMusicHandler::init()
{
	CAudioBase::init();

	if (initialized)
		Mix_HookMusicFinished(musicFinishedCallbackC);
}

void CMusicHandler::release()
{
	if (initialized)
	{
		boost::mutex::scoped_lock guard(musicMutex);

		Mix_HookMusicFinished(NULL);

		current.reset();
		next.reset();
	}

	CAudioBase::release();
}

void CMusicHandler::playMusic(std::string musicURI, bool loop)
{
	if (current && current->isTrack( musicURI))
		return;

	queueNext(new MusicEntry(this, "", musicURI, loop));
}

void CMusicHandler::playMusicFromSet(std::string whichSet, bool loop)
{
	auto selectedSet = musicsSet.find(whichSet);
	if (selectedSet == musicsSet.end())
	{
		tlog0 << "Error: playing music from non-existing set: " << whichSet << "\n";
		return;
	}

	if (current && current->isSet(whichSet))
		return;

	queueNext(new MusicEntry(this, whichSet, "", loop));
}


void CMusicHandler::playMusicFromSet(std::string whichSet, int entryID, bool loop)
{
	auto selectedSet = musicsSet.find(whichSet);
	if (selectedSet == musicsSet.end())
	{
		tlog0 << "Error: playing music from non-existing set: " << whichSet << "\n";
		return;
	}

	auto selectedEntry = selectedSet->second.find(entryID);
	if (selectedEntry == selectedSet->second.end())
	{
		tlog0 << "Error: playing non-existing entry " << entryID << " from set: " << whichSet << "\n";
		return;
	}
	queueNext(new MusicEntry(this, "", selectedEntry->second, loop));
}

void CMusicHandler::queueNext(MusicEntry *queued)
{
	if (!initialized)
		return;

	boost::mutex::scoped_lock guard(musicMutex);

	next.reset(queued);

	if (current.get() != NULL)
	{
		current->stop(1000);
	}
	else
	{
		current.reset(next.release());
		current->play();
	}
}

void CMusicHandler::stopMusic(int fade_ms)
{
	if (!initialized)
		return;

	boost::mutex::scoped_lock guard(musicMutex);

	if (current.get() != NULL)
		current->stop(fade_ms);
	next.reset();
}

void CMusicHandler::setVolume(ui32 percent)
{
	CAudioBase::setVolume(percent);

	if (initialized)
		Mix_VolumeMusic((MIX_MAX_VOLUME * volume)/100);
}

void CMusicHandler::musicFinishedCallback(void)
{
	boost::mutex::scoped_lock guard(musicMutex);

	if (current.get() != NULL)
	{
		//return if current music still not finished
		if (current->play())
			return;
		else
			current.reset();
	}

	if (current.get() == NULL && next.get() != NULL)
	{
		current.reset(next.release());
		current->play();
	}
}

MusicEntry::MusicEntry(CMusicHandler *owner, std::string setName, std::string musicURI, bool looped):
	owner(owner),
	music(nullptr),
	looped(looped),
    setName(setName)
{
	if (!musicURI.empty())
		load(musicURI);
}
MusicEntry::~MusicEntry()
{
	tlog5<<"Del-ing music file "<<currentName<<"\n";
	if (music)
		Mix_FreeMusic(music);
}

void MusicEntry::load(std::string musicURI)
{
	if (music)
	{
		tlog5<<"Del-ing music file "<<currentName<<"\n";
		Mix_FreeMusic(music);
	}

	currentName = musicURI;

	tlog5<<"Loading music file "<<musicURI<<"\n";

	music = Mix_LoadMUS(CResourceHandler::get()->getResourceName(ResourceID(musicURI, EResType::MUSIC)).c_str());

	if(!music)
	{
		tlog3 << "Warning: Cannot open " << currentName << ": " << Mix_GetError() << std::endl;
		return;
	}

#ifdef _WIN32
	//The assertion will fail if old MSVC libraries pack .dll is used
	assert(Mix_GetMusicType(music) != MUS_MP3);
#endif
}

bool MusicEntry::play()
{
	if (!looped && music) //already played once - return
		return false;

	if (!setName.empty())
	{
		auto set = owner->musicsSet[setName];
		size_t entryID = rand() % set.size();
		auto iterator = set.begin();
		std::advance(iterator, entryID);
		load(iterator->second);
	}

	tlog5<<"Playing music file "<<currentName<<"\n";
	if(Mix_PlayMusic(music, 1) == -1)
	{
		tlog1 << "Unable to play music (" << Mix_GetError() << ")" << std::endl;
		return false;
	}
	return true;
}

void MusicEntry::stop(int fade_ms)
{
	tlog5<<"Stoping music file "<<currentName<<"\n";
	looped = false;
	Mix_FadeOutMusic(fade_ms);
}

bool MusicEntry::isSet(std::string set)
{
	return !setName.empty() && set == setName;
}

bool MusicEntry::isTrack(std::string track)
{
	return setName.empty() && track == currentName;
}
