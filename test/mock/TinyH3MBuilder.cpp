/*
 * TinyH3MBuilder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TinyH3MBuilder.h"

#include "TinyH3MWriter.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/texts/CLegacyConfigParser.h"
#include "../../lib/mapping/MapFeaturesH3M.h"
#include "../../lib/mapping/MapFormatSettings.h"
#include "../../lib/texts/TextOperations.h"

#include <boost/algorithm/string.hpp>
#include <zlib.h>

namespace TinyH3M
{

namespace
{

// One row of Data/Objects.txt, parsed verbatim. Used as the source of truth for
// template bytes — we re-emit the same 9 fields the H3 editor authored.
struct LegacyTemplate
{
	std::string animationFile;
	std::string blockBits;     // 48 chars '0'/'1'
	std::string visitBits;     // 48 chars '0'/'1'
	std::string unknownBits;   // 9 chars (Objects.txt strings[3])
	std::string terrainBits;   // 9 chars (Objects.txt strings[4])
	int         id            = 0;
	int         subid         = 0;
	int         type          = 0;
	int         printPriority = 0;
};

using LegacyKey = std::pair<int, int>;

const std::map<LegacyKey, LegacyTemplate> & legacyTemplates()
{
	// Heroes.txt mirrors Objects.txt format and ships the HERO (id=34, subid=0..17)
	// and RANDOM_HERO (id=70) template rows that Objects.txt omits.
	static const auto cache = []
	{
		std::map<LegacyKey, LegacyTemplate> out;
		auto loadFile = [&](const std::string & resourcePath)
		{
			CLegacyConfigParser parser(TextPath::builtin(resourcePath));
			const auto total = static_cast<size_t>(parser.readNumber());
			parser.endLine();

			for(size_t i = 0; i < total; ++i)
			{
				const std::string data = parser.readString();
				std::vector<std::string> fields;
				boost::split(fields, data, boost::is_any_of(" "));
				assert(fields.size() == 9);

				LegacyTemplate t;
				t.animationFile = fields[0];
				t.blockBits     = fields[1];
				t.visitBits     = fields[2];
				t.unknownBits   = fields[3];
				t.terrainBits   = fields[4];
				t.id            = std::stoi(fields[5]);
				t.subid         = std::stoi(fields[6]);
				t.type          = std::stoi(fields[7]);
				t.printPriority = std::stoi(fields[8]);

				out.emplace(LegacyKey(t.id, t.subid), t);
				parser.endLine();
			}
		};
		loadFile("Data/Objects.txt");
		loadFile("Data/Heroes.txt");
		return out;
	}();
	return cache;
}

const LegacyTemplate & legacyTemplate(MapObjectID id, MapObjectSubID subid)
{
	const auto & cache = legacyTemplates();
	const auto it = cache.find(LegacyKey(id.getNum(), subid.getNum()));
	if(it == cache.end())
		throw std::runtime_error("TinyH3MBuilder: no Data/Objects.txt entry for id="
			+ std::to_string(id.getNum()) + " subid=" + std::to_string(subid.getNum()));
	return it->second;
}

// Wire blockMask[i] bit j set iff blockStr[(5-i)*8 + (7-j)] == '1'.
// (See ObjectTemplate::readMap vs ObjectTemplate::readTxt.)
uint8_t packBlockOrVisitByte(const std::string & bits48, int wireRow, bool wantBit)
{
	uint8_t byte = 0;
	for(int j = 0; j < 8; ++j)
	{
		const char ch = bits48[(5 - wireRow) * 8 + (7 - j)];
		if((ch == '1') == wantBit)
			byte |= static_cast<uint8_t>(1 << j);
	}
	return byte;
}

uint16_t packTerrainMask(const std::string & bits9)
{
	// Per ObjectTemplate::readTxt: terrain i allowed iff bits9[8-i] == '1'.
	// Per ObjectTemplate::readMap: terrain i allowed iff bit i of terrMask is 1.
	uint16_t mask = 0;
	for(int i = 0; i < 9; ++i)
	{
		if(bits9[8 - i] == '1')
			mask |= static_cast<uint16_t>(1 << i);
	}
	return mask;
}

void writeLegacyTemplate(TinyH3MWriter & w, const LegacyTemplate & t)
{
	w.writeBaseString(t.animationFile);
	for(int row = 0; row < 6; ++row)
		w.writeUInt8(packBlockOrVisitByte(t.blockBits, row, /*wantBit*/ true));
	for(int row = 0; row < 6; ++row)
		w.writeUInt8(packBlockOrVisitByte(t.visitBits, row, /*wantBit*/ true));

	// The first uint16 is read-and-discarded by the loader. Real maps seem to
	// hold editor-cached data here; encoding the unknownBits field the same way
	// we encode terrainBits is a best-effort match and the loader doesn't care.
	w.writeUInt16(packTerrainMask(t.unknownBits));
	w.writeUInt16(packTerrainMask(t.terrainBits));

	w.writeUInt32(static_cast<uint32_t>(t.id));
	w.writeUInt32(static_cast<uint32_t>(t.subid));
	w.writeUInt8(static_cast<uint8_t>(t.type));
	w.writeUInt8(static_cast<uint8_t>(t.printPriority / 100));
	w.skipZero(16);
}

// Per-format field sizes. The HotA sub-version only matters for HOTA maps; every
// other format ignores it. Builder and loader must agree on this exact value or
// the sized bitmasks desync the stream.
MapFormatFeaturesH3M featuresFor(EMapFormat format, uint32_t hotaVersion)
{
	return MapFormatFeaturesH3M::find(format, format == EMapFormat::HOTA ? hotaVersion : 0);
}

} // namespace

TinyH3MBuilder::TinyH3MBuilder(EMapFormat format_)
	: format(format_)
{
}

TinyH3MBuilder & TinyH3MBuilder::size(int sideLength_, bool twoLevel_)
{
	sideLength = sideLength_;
	twoLevel = twoLevel_;
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::name(std::string s)
{
	mapName = std::move(s);
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::description(std::string s)
{
	mapDescription = std::move(s);
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::difficulty(EMapDifficulty d)
{
	mapDifficulty = d;
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::hotaVersion(uint32_t version)
{
	hotaFormatVersion = version;
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::playerActive(PlayerColor color)
{
	playerEnabled.at(color.getNum()) = true;
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::town(const int3 & pos, FactionID faction, PlayerColor owner)
{
	ObjectSpec spec;
	spec.id            = Obj::TOWN;
	spec.subid         = MapObjectSubID(faction.getNum());
	spec.position      = pos;
	spec.owner         = owner;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::randomTown(const int3 & pos, PlayerColor owner)
{
	ObjectSpec spec;
	spec.id            = Obj::RANDOM_TOWN;
	spec.subid         = MapObjectSubID(0);
	spec.position      = pos;
	spec.owner         = owner;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::hero(const int3 & pos, HeroTypeID type, PlayerColor owner)
{
	ObjectSpec spec;
	spec.id            = Obj::HERO;
	// The HERO object template is keyed by hero *class* (Heroes.txt ships one row per
	// class, subid 0..17). The hero's actual identity is written separately in the
	// object body (writeHeroBody / CMapLoaderH3M::readHero) and is what the loader uses.
	spec.subid         = MapObjectSubID(type.toHeroType()->heroClass->getIndex());
	spec.position      = pos;
	spec.owner         = owner;
	spec.heroType      = type;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::randomHero(const int3 & pos, PlayerColor owner)
{
	ObjectSpec spec;
	spec.id            = Obj::RANDOM_HERO;
	spec.subid         = MapObjectSubID(0);
	spec.position      = pos;
	spec.owner         = owner;
	spec.heroType      = HeroTypeID(-1); // wire sentinel = features.heroIdentifierInvalid
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::heroGarrison(std::vector<std::pair<CreatureID, uint16_t>> stacks)
{
	auto & spec = lastObject();
	assert(spec.id == Obj::HERO || spec.id == Obj::RANDOM_HERO);
	spec.heroGarrisonStacks = std::move(stacks);
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::heroExperience(uint32_t totalXp)
{
	auto & spec = lastObject();
	assert(spec.id == Obj::HERO || spec.id == Obj::RANDOM_HERO);
	spec.heroExperienceXp = totalXp;
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::heroPrimary(uint8_t attack, uint8_t defense, uint8_t spellPower, uint8_t knowledge)
{
	auto & spec = lastObject();
	assert(spec.id == Obj::HERO || spec.id == Obj::RANDOM_HERO);
	spec.heroPrimarySkills = std::array<uint8_t, 4>{attack, defense, spellPower, knowledge};
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::heroSecondarySkills(std::vector<std::pair<SecondarySkill, uint8_t>> skills)
{
	auto & spec = lastObject();
	assert(spec.id == Obj::HERO || spec.id == Obj::RANDOM_HERO);
	spec.heroSecondarySkills = std::move(skills);
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::heroSpells(std::vector<SpellID> spells)
{
	auto & spec = lastObject();
	assert(spec.id == Obj::HERO || spec.id == Obj::RANDOM_HERO);
	spec.heroSpells = std::move(spells);
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::heroEquipped(std::vector<std::pair<ArtifactPosition, ArtifactID>> equipped)
{
	auto & spec = lastObject();
	assert(spec.id == Obj::HERO || spec.id == Obj::RANDOM_HERO);
	spec.heroEquippedArts = std::move(equipped);
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::heroBackpack(std::vector<ArtifactID> backpack)
{
	auto & spec = lastObject();
	assert(spec.id == Obj::HERO || spec.id == Obj::RANDOM_HERO);
	spec.heroBackpackArts = std::move(backpack);
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::monster(const int3 & pos, CreatureID creature, uint16_t count, int8_t character)
{
	ObjectSpec spec;
	spec.id               = Obj::MONSTER;
	spec.subid            = MapObjectSubID(creature.getNum());
	spec.position         = pos;
	spec.monsterCount     = count;
	spec.monsterCharacter = character;
	spec.templateIndex    = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::resource(const int3 & pos, GameResID resource, uint32_t amount)
{
	ObjectSpec spec;
	spec.id             = Obj::RESOURCE;
	spec.subid          = MapObjectSubID(resource.getNum());
	spec.position       = pos;
	spec.resourceAmount = amount;
	spec.templateIndex  = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::pandora(const int3 & pos)
{
	ObjectSpec spec;
	spec.id            = Obj::PANDORAS_BOX;
	spec.subid         = MapObjectSubID(0);
	spec.position      = pos;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::artifact(const int3 & pos, ArtifactID artifact)
{
	ObjectSpec spec;
	spec.id            = Obj::ARTIFACT;
	spec.subid         = MapObjectSubID(artifact.getNum());
	spec.position      = pos;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::scroll(const int3 & pos, SpellID spell)
{
	ObjectSpec spec;
	spec.id            = Obj::SPELL_SCROLL;
	spec.subid         = MapObjectSubID(0);
	spec.position      = pos;
	spec.scrollSpell   = spell;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::dwelling(const int3 & pos, MapObjectSubID type, PlayerColor owner)
{
	ObjectSpec spec;
	spec.id            = Obj::CREATURE_GENERATOR1;
	spec.subid         = type;
	spec.position      = pos;
	spec.owner         = owner;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::keymaster(const int3 & pos, int color)
{
	ObjectSpec spec;
	spec.id            = Obj::KEYMASTER;
	spec.subid         = MapObjectSubID(color);
	spec.position      = pos;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::borderGuard(const int3 & pos, int color)
{
	ObjectSpec spec;
	spec.id            = Obj::BORDERGUARD;
	spec.subid         = MapObjectSubID(color);
	spec.position      = pos;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::borderGate(const int3 & pos, int color)
{
	ObjectSpec spec;
	spec.id            = Obj::BORDER_GATE;
	spec.subid         = MapObjectSubID(color);
	spec.position      = pos;
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::questGuard(const int3 & pos, Quest mission)
{
	ObjectSpec spec;
	spec.id            = Obj::QUEST_GUARD;
	spec.subid         = MapObjectSubID(0);
	spec.position      = pos;
	spec.quest         = std::move(mission);
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::questGate(const int3 & pos, Quest mission)
{
	// HotA encodes the Quest Gate as a BORDER_GATE with the magic subID 1000.
	ObjectSpec spec;
	spec.id            = Obj::BORDER_GATE;
	spec.subid         = MapObjectSubID(1000);
	spec.position      = pos;
	spec.quest         = std::move(mission);
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::seerHut(const int3 & pos, Quest mission, SeerReward reward)
{
	ObjectSpec spec;
	spec.id            = Obj::SEER_HUT;
	spec.subid         = MapObjectSubID(0);
	spec.position      = pos;
	spec.seerOneShots.emplace_back(std::move(mission), reward);
	spec.templateIndex = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

TinyH3MBuilder & TinyH3MBuilder::seerHutMulti(const int3 & pos,
	std::vector<std::pair<Quest, SeerReward>> oneShots,
	std::vector<std::pair<Quest, SeerReward>> repeatables)
{
	ObjectSpec spec;
	spec.id              = Obj::SEER_HUT;
	spec.subid           = MapObjectSubID(0);
	spec.position        = pos;
	spec.seerOneShots    = std::move(oneShots);
	spec.seerRepeatables = std::move(repeatables);
	spec.templateIndex   = registerTemplate(spec.id, spec.subid);
	registerObject(std::move(spec));
	return *this;
}

void TinyH3MBuilder::registerObject(ObjectSpec spec)
{
	spec.wireIdentifier = nextWireIdentifier++;
	objects.push_back(std::move(spec));
}

TinyH3MBuilder::ObjectSpec & TinyH3MBuilder::lastObject()
{
	assert(!objects.empty() && "lastObject() called before any object was added");
	return objects.back();
}

ObjectHandle TinyH3MBuilder::lastHandle() const
{
	assert(!objects.empty() && "lastHandle() called before any object was added");
	return ObjectHandle{objects.back().wireIdentifier};
}

Quest TinyH3MBuilder::missionArtifacts(std::vector<ArtifactID> artifacts)
{
	Quest q;
	q.kind = EQuestMission::ARTIFACT;
	q.artifacts = std::move(artifacts);
	return q;
}

Quest TinyH3MBuilder::missionArmy(std::vector<std::pair<CreatureID, uint16_t>> stacks)
{
	Quest q;
	q.kind = EQuestMission::ARMY;
	q.creatures = std::move(stacks);
	return q;
}

Quest TinyH3MBuilder::missionResources(std::array<uint32_t, 7> amounts)
{
	Quest q;
	q.kind = EQuestMission::RESOURCES;
	q.resources = amounts;
	return q;
}

Quest TinyH3MBuilder::missionPrimarySkills(uint8_t attack, uint8_t defense, uint8_t spellPower, uint8_t knowledge)
{
	Quest q;
	q.kind = EQuestMission::PRIMARY_SKILL;
	q.primarySkills = {attack, defense, spellPower, knowledge};
	return q;
}

Quest TinyH3MBuilder::missionLevel(uint32_t level)
{
	Quest q;
	q.kind = EQuestMission::LEVEL;
	q.heroLevel = level;
	return q;
}

Quest TinyH3MBuilder::missionHero(HeroTypeID hero)
{
	Quest q;
	q.kind = EQuestMission::HERO;
	q.hero = hero;
	return q;
}

Quest TinyH3MBuilder::missionPlayer(PlayerColor player)
{
	Quest q;
	q.kind = EQuestMission::PLAYER;
	q.player = player;
	return q;
}

Quest TinyH3MBuilder::missionKillCreature(ObjectHandle target)
{
	Quest q;
	q.kind = EQuestMission::KILL_CREATURE;
	q.killTargetIdentifier = target.wireIdentifier;
	return q;
}

Quest TinyH3MBuilder::missionKillHero(ObjectHandle target)
{
	Quest q;
	q.kind = EQuestMission::KILL_HERO;
	q.killTargetIdentifier = target.wireIdentifier;
	return q;
}

Quest TinyH3MBuilder::missionHeroClass(std::vector<HeroClassID> classes)
{
	Quest q;
	q.kind = EQuestMission::HOTA_HERO_CLASS;
	q.heroClasses = std::move(classes);
	return q;
}

Quest TinyH3MBuilder::missionReachDate(uint32_t daysPassed)
{
	assert(daysPassed >= 1); // loader stores wireValue + 1, so day 0 is unreachable
	Quest q;
	q.kind = EQuestMission::HOTA_REACH_DATE;
	q.reachDateDay = daysPassed;
	return q;
}

Quest TinyH3MBuilder::missionDifficulty(uint8_t difficultyMask)
{
	assert(difficultyMask >= 1 && difficultyMask <= 31);
	Quest q;
	q.kind = EQuestMission::HOTA_GAME_DIFFICULTY;
	q.difficultyMask = difficultyMask;
	return q;
}

Quest TinyH3MBuilder::missionScripted(uint32_t scriptEventID)
{
	Quest q;
	q.kind = EQuestMission::HOTA_SCRIPTED;
	q.scriptEventID = scriptEventID;
	return q;
}

SeerReward TinyH3MBuilder::rewardNothing()
{
	return {};
}

SeerReward TinyH3MBuilder::rewardExperience(uint32_t amount)
{
	SeerReward r;
	r.kind = SeerReward::Kind::EXPERIENCE;
	r.amount = amount;
	return r;
}

SeerReward TinyH3MBuilder::rewardMana(uint32_t amount)
{
	SeerReward r;
	r.kind = SeerReward::Kind::MANA;
	r.amount = amount;
	return r;
}

SeerReward TinyH3MBuilder::rewardResource(GameResID resource, uint32_t amount)
{
	SeerReward r;
	r.kind = SeerReward::Kind::RESOURCES;
	r.resourceType = resource;
	r.resourceAmount = amount;
	return r;
}

uint32_t TinyH3MBuilder::registerTemplate(MapObjectID id, MapObjectSubID subid)
{
	const auto key = std::make_pair(id, subid);
	for(size_t i = 0; i < templates.size(); ++i)
	{
		if(templates[i] == key)
			return static_cast<uint32_t>(i);
	}
	templates.push_back(key);
	return static_cast<uint32_t>(templates.size() - 1);
}

std::vector<uint8_t> TinyH3MBuilder::build()
{
	if(format == EMapFormat::HOTA && hotaFormatVersion > 3)
		throw std::runtime_error("TinyH3MBuilder: only HotA sub-format versions 0..3 are implemented");

	TinyH3MWriter w;
	auto features = featuresFor(format, hotaFormatVersion);
	w.setFormatLevel(features);

	writeHeader(w);
	writeMapOptions(w);
	// CMapLoaderH3M::readHotaScripts is HOTA9+ only; builder does not emit HOTA yet.
	writeAllowedArtifacts(w);
	writeAllowedSpellsAbilities(w);
	writeRumors(w);
	writePredefinedHeroes(w);
	writeTerrain(w);
	writeObjectTemplates(w);
	writeObjects(w);
	writeEvents(w);

	return w.take();
}

std::vector<uint8_t> TinyH3MBuilder::buildAndDump(const std::string & testName)
{
	mapName = testName; // so the map identifies its originating test when opened in the editor
	auto bytes = build();

	const auto dir = VCMIDirs::get().userCachePath() / "testMaps";
	boost::filesystem::create_directories(dir);
	const auto path = dir / (testName + ".h3m");

	// Real .h3m files on disk are gzip-wrapped. zlib's gzFile API writes that
	// wrapping directly; no need to roll our own deflateInit2(windowBits=31).
	gzFile gz = gzopen(path.string().c_str(), "wb");
	if(gz == nullptr)
		throw std::runtime_error("TinyH3MBuilder: failed to open " + path.string() + " for writing");

	const int written = gzwrite(gz, bytes.data(), static_cast<unsigned int>(bytes.size()));
	gzclose(gz);

	if(written != static_cast<int>(bytes.size()))
		throw std::runtime_error("TinyH3MBuilder: short gzwrite to " + path.string());

	return bytes;
}

void TinyH3MBuilder::writeHeader(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// Mirror of CMapLoaderH3M::readHeader.
	w.writeUInt32(static_cast<uint32_t>(format));   // EMapFormat byte read as uint32

	if(format == EMapFormat::HOTA)
	{
		w.writeUInt32(hotaFormatVersion);
		if(features.levelHOTA1)
		{
			w.writeBool(false); // isMirrorMap
			w.writeBool(false); // isArenaMap
		}
		if(features.levelHOTA2)
			w.writeUInt32(static_cast<uint32_t>(features.terrainsCount));
		// levelHOTA5+ header blocks (town-types/difficulty mask, hire-defeated, ...)
		// are not emitted: only HotA versions 0..3 are supported by the builder.
	}

	// areAnyPlayers must be false when no human/computer can play any color, otherwise
	// the H3 editor rejects the map.
	const bool anyPlayer = std::any_of(playerEnabled.begin(), playerEnabled.end(),
		[](bool b) { return b; });
	w.writeBool(anyPlayer);
	w.writeInt32(sideLength);                       // height = width
	w.writeBool(twoLevel);
	w.writeBaseString(mapName);
	w.writeBaseString(mapDescription);
	w.writeUInt8(static_cast<uint8_t>(mapDifficulty));

	if(features.levelAB)
		w.writeUInt8(/*levelLimit*/ 0);

	writePlayerInfo(w);
	writeStandardVictoryLoss(w);
	writeTeamInfo(w);
	writeAllowedHeroes(w);
	writeDisposedHeroes(w);
}

void TinyH3MBuilder::writePlayerInfo(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// All 8 players default to "neither human nor computer".
	//
	// The loader takes a `skipUnused` shortcut for inactive players, but the H3
	// editor still parses the bytes against the active-player schema (so it can
	// remember "what would happen if this player were activated"). We must write
	// canonical inactive values, otherwise the editor refuses to open the map.
	//
	// Wire layout for an active player at its minimum size (no main town,
	// no custom hero), matching CMapLoaderH3M::readPlayerInfo:
	//   aiTactic              uint8        (0 = AI_RANDOM)
	//   factionSelectable     uint8 (SOD)  (0)
	//   allowedFactions       bitmask      (factionsBytes; 0 = none)
	//   isFactionRandom       bool         (0)
	//   hasMainTown           bool         (0)
	//   hasRandomHero         bool         (0)
	//   mainCustomHeroId      uint8        (heroIdentifierInvalid = NONE)
	//   skipUnused            uint8 (AB+)  (0)
	//   placeholdersCount     uint32 (AB+) (0)
	//
	// Byte totals: ROE=6, AB=12, SOD=13 — match the loader's skip path exactly.
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		const bool active = playerEnabled[i];

		w.writeBool(active); // canHumanPlay
		w.writeBool(active); // canComputerPlay

		w.writeUInt8(0);                                // aiTactic = AI_RANDOM
		if(features.levelSOD)
			w.writeUInt8(active ? 0xff : 0);            // SOD: faction-selectable flag
		// allowedFactions bitmask: all-allowed for active, none for inactive
		if(active)
			w.writeAllOnes(features.factionsBytes);
		else
			w.skipZero(features.factionsBytes);
		w.writeBool(false);                             // isFactionRandom
		w.writeBool(false);                             // hasMainTown
		w.writeBool(false);                             // hasRandomHero
		w.writeUInt8(static_cast<uint8_t>(features.heroIdentifierInvalid)); // mainCustomHeroId = NONE
		if(features.levelAB)
		{
			w.writeUInt8(0);                            // AB: unknown skip byte
			w.writeUInt32(0);                           // AB: placeholders count
		}
	}
}

void TinyH3MBuilder::writeStandardVictoryLoss(TinyH3MWriter & w) const
{
	// Standard win/loss = -1 sentinel byte each; no payload follows.
	w.writeInt8(-1); // EVictoryConditionType::WINSTANDARD
	w.writeInt8(-1); // ELossConditionType::LOSSSTANDARD
}

void TinyH3MBuilder::writeTeamInfo(TinyH3MWriter & w) const
{
	// howManyTeams == 0 skips the per-player team table.
	w.writeUInt8(0);
}

void TinyH3MBuilder::writeAllowedHeroes(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// non-HOTA: fixed-size bitmask (features.heroesBytes bytes)
	// HOTA:     size-prefixed bitmask (uint32 count + ceil(count/8) bytes)
	if(features.levelHOTA0)
	{
		// size-prefixed: a count of 0 leaves the default-allowed set untouched.
		// (Emitting the full HotA heroesCount would index entities the test
		// environment lacks without the HotA mod loaded.)
		w.writeUInt32(0);
	}
	else
	{
		w.writeAllOnes(features.heroesBytes);
	}

	if(features.levelAB)
	{
		// placeholdersQty + per-entry readHero (1 byte each). 0 placeholders.
		w.writeUInt32(0);
	}
}

void TinyH3MBuilder::writeDisposedHeroes(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// SOD+ only, single count byte (no entries follow when count == 0).
	if(features.levelSOD)
		w.writeUInt8(0);
}

void TinyH3MBuilder::writeMapOptions(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// 31 reserved zero bytes.
	w.skipZero(31);

	if(features.levelHOTA0)
	{
		w.writeBool(false); // allowSpecialMonths
		w.skipZero(3);
	}
	if(features.levelHOTA1)
		w.writeInt32(0);    // combinedArtifactsCount = none banned (no bitmask bytes follow)
	if(features.levelHOTA3)
		w.writeInt32(-1);   // roundLimit = no limit
	// levelHOTA5+ per-player hero-recruitment block not emitted (versions 0..3 only).
}

void TinyH3MBuilder::writeAllowedArtifacts(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	//   ROE: no bytes (not AB+).
	//   AB / SOD: bitmask of features.artifactsBytes (non-HOTA).
	//   HOTA0+: size-prefixed bitmask (uint32 count + ceil(count/8) bytes).
	// invert=true on the read side, so all-zero bytes here means "default-allowed
	// set untouched" -> all standard artifacts remain allowed.
	if(features.levelHOTA0)
	{
		// size-prefixed: a count of 0 keeps the default-allowed artifact set.
		w.writeUInt32(0);
	}
	else if(features.levelAB)
		w.skipZero(features.artifactsBytes);
}

void TinyH3MBuilder::writeAllowedSpellsAbilities(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// SOD+ only. Same invert=true convention -> zero bytes means default-allowed.
	if(features.levelSOD)
	{
		w.skipZero(features.spellsBytes);
		w.skipZero(features.skillsBytes);
	}
}

void TinyH3MBuilder::writeRumors(TinyH3MWriter & w) const
{
	// uint32 count + per-entry name + text. No rumors emitted.
	w.writeUInt32(0);
}

void TinyH3MBuilder::writePredefinedHeroes(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	//   non-SOD: nothing to emit.
	//   SOD: one `customised` bool per hero (all false = no overrides).
	//   HOTA0+: prefix with uint32 heroesCount.
	//   HOTA5+: trailing per-hero block (not emitted; versions 0..3 only).
	if(!features.levelSOD)
		return;

	if(features.levelHOTA0)
	{
		// HOTA prefixes the per-hero "customised" flags with their count; 0 = none.
		w.writeUInt32(0);
		return;
	}
	w.skipZero(features.heroesCount);
}

void TinyH3MBuilder::writeTerrain(TinyH3MWriter & w) const
{
	// Per tile (iterated z-y-x): 7 bytes — terrainType, terView, riverType,
	// riverDir, roadType, roadDir, extTileFlags.
	//
	// H3M raw terrain byte uses the H3 ordering DIRT=0, SAND=1, GRASS=2, ...
	// (which happens to match VCMI's TerrainId enum). Writing 0 would mean DIRT;
	// for a blank grass map we need 2.
	//
	// terView = 0 maps to an "edge of biome" tile in the H3 editor, which looks
	// wrong though it loads fine. Use view 0x31 (49) — a known plain-grass tile
	// from H3's tileset. Real maps put pseudo-random view bytes here.
	const int levels = twoLevel ? 2 : 1;
	const size_t tiles = static_cast<size_t>(sideLength) * sideLength * levels;
	for(size_t i = 0; i < tiles; ++i)
	{
		w.writeUInt8(2);    // terrain type = GRASS in H3M ordering
		w.writeUInt8(0x31); // terView — plain-grass tile
		w.writeUInt8(0);    // river type
		w.writeUInt8(0);    // river dir
		w.writeUInt8(0);    // road type
		w.writeUInt8(0);    // road dir
		w.writeUInt8(0);    // extTileFlags
	}
}

void TinyH3MBuilder::writeObjectTemplates(TinyH3MWriter & w) const
{
	// uint32 count + per-template body.
	w.writeUInt32(static_cast<uint32_t>(templates.size()));
	for(const auto & key : templates)
	{
		// The HotA Quest Gate (BORDER_GATE subID 1000) has no Objects.txt row;
		// reuse the colourless BORDER_GATE template bytes with the subID overridden.
		if(key.first == Obj::BORDER_GATE && key.second.getNum() == 1000)
		{
			LegacyTemplate t = legacyTemplate(Obj::BORDER_GATE, MapObjectSubID(0));
			t.subid = 1000;
			writeLegacyTemplate(w, t);
		}
		else
		{
			writeLegacyTemplate(w, legacyTemplate(key.first, key.second));
		}
	}
}

void TinyH3MBuilder::writeObjects(TinyH3MWriter & w) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// uint32 count + per-object body. Per-object header is int3 position +
	// uint32 template index + 5 reserved bytes, then a type-specific body.
	w.writeUInt32(static_cast<uint32_t>(objects.size()));

	for(const auto & obj : objects)
	{
		w.writeInt3(obj.position);
		w.writeUInt32(obj.templateIndex);
		w.skipZero(5);

		// Minimum-viable body per type: empty / "use defaults" wherever possible.
		// Mirror of CMapLoaderH3M::read{Town,Monster,Resource,Artifact,...}.
		switch(obj.id.toEnum())
		{
			case Obj::RANDOM_TOWN:
			case Obj::TOWN:
				// No garrison, standard fort, no events, no custom buildings, neutral alignment.
				if(features.levelAB)
					w.writeUInt32(obj.wireIdentifier);                 // identifier
				w.writePlayer(obj.owner);                              // owner
				w.writeBool(false);                                    // hasName
				w.writeBool(false);                                    // hasGarrison
				w.writeInt8(0);                                        // formation = LOOSE
				w.writeBool(false);                                    // hasCustomBuildings
				w.writeBool(true);                                     // hasFort
				if(features.levelAB)
					w.skipZero(features.spellsBytes);                  // obligatorySpells bitmask
				w.skipZero(features.spellsBytes);                      // possibleSpells bitmask
				if(features.levelHOTA1)
					w.writeBool(false);                                // spellResearchAllowed
				w.writeUInt32(0);                                      // castle events count
				if(features.levelSOD)
					w.writeUInt8(0xff);                                // alignment = "same as owner / random"
				w.skipZero(3);
				break;

			case Obj::MONSTER:
			case Obj::RANDOM_MONSTER:
			case Obj::RANDOM_MONSTER_L1:
			case Obj::RANDOM_MONSTER_L2:
			case Obj::RANDOM_MONSTER_L3:
			case Obj::RANDOM_MONSTER_L4:
			case Obj::RANDOM_MONSTER_L5:
			case Obj::RANDOM_MONSTER_L6:
			case Obj::RANDOM_MONSTER_L7:
				if(features.levelAB)
					w.writeUInt32(obj.wireIdentifier);                 // identifier (referenced by kill-creature quests)
				w.writeUInt16(obj.monsterCount);
				w.writeInt8(obj.monsterCharacter);
				w.writeBool(false);                                    // hasMessage
				w.writeBool(false);                                    // neverFlees
				w.writeBool(false);                                    // notGrowingTeam
				w.skipZero(2);
				break;

			case Obj::RESOURCE:
			case Obj::RANDOM_RESOURCE:
				w.writeBool(false);                                    // hasMessage
				w.writeUInt32(obj.resourceAmount);
				w.skipZero(4);
				break;

			case Obj::PANDORAS_BOX:
				// Mirror of readBoxContent, readPandora and readBoxHotaContent
				w.writeBool(false);                                    // hasMessage (=> no guards / no skip)
				w.writeUInt32(0);                                      // heroExperience
				w.writeInt32(0);                                       // manaDiff
				w.writeInt8(0);                                        // morale
				w.writeInt8(0);                                        // luck
				for(int i = 0; i < features.resourcesCount; ++i)
					w.writeInt32(0);                                   // resources
				for(int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
					w.writeUInt8(0);                                   // primary skills
				w.writeUInt8(0);                                       // gained abilities count
				w.writeUInt8(0);                                       // gained artifacts count
				w.writeUInt8(0);                                       // gained spells count
				w.writeUInt8(0);                                       // gained creatures count
				w.skipZero(8);                                         // reserved

				if(features.levelHOTA5)
				{
					w.skipZero(1);                                     // readPandora: unknown, always 0
					w.writeInt32(0);                                   // movement mode: 0 = give
					w.writeInt32(0);                                   // movement amount
				}
				if(features.levelHOTA6)
					w.writeInt32(31);                                  // allowed difficulties; reader rejects 0
				if(features.levelHOTA9)
					w.writeBool(false);                                // does not use the event system
				break;

			case Obj::ARTIFACT:
			case Obj::RANDOM_ART:
			case Obj::RANDOM_TREASURE_ART:
			case Obj::RANDOM_MINOR_ART:
			case Obj::RANDOM_MAJOR_ART:
			case Obj::RANDOM_RELIC_ART:
				w.writeBool(false);                                    // hasMessage
				break;

			case Obj::HERO:
			case Obj::RANDOM_HERO:
				writeHeroBody(w, obj);
				break;

			case Obj::SPELL_SCROLL:
				writeScrollBody(w, obj);
				break;

			case Obj::CREATURE_GENERATOR1:
				w.writePlayer32(obj.owner);
				break;

			case Obj::KEYMASTER:
			case Obj::BORDERGUARD:
				// readGeneric — no body bytes. Subid (keymaster colour) lives in the template.
				break;

			case Obj::BORDER_GATE:
				// HotA Quest Gate (subID 1000) carries a quest body, read via
				// readQuestGuard. A plain border gate has no body (readGeneric).
				if(obj.subid.getNum() == 1000)
					writeQuestBody(w, obj.quest);
				break;

			case Obj::QUEST_GUARD:
				writeQuestBody(w, obj.quest);
				break;

			case Obj::SEER_HUT:
			{
				// HOTA v3+: uint32 one-shot count, the quests, uint32 repeatable
				// count, the repeatables, then skipZero(2). Pre-HOTA: exactly one
				// quest and the trailing skipZero(2).
				const bool multiQuest = features.levelHOTA3;
				assert((multiQuest || (obj.seerOneShots.size() == 1 && obj.seerRepeatables.empty()))
					&& "multi-quest seer huts require the HOTA v3+ format");

				if(multiQuest)
					w.writeUInt32(static_cast<uint32_t>(obj.seerOneShots.size()));
				for(const auto & [quest, reward] : obj.seerOneShots)
					writeSeerHutQuest(w, quest, reward);

				if(multiQuest)
				{
					w.writeUInt32(static_cast<uint32_t>(obj.seerRepeatables.size()));
					for(const auto & [quest, reward] : obj.seerRepeatables)
						writeSeerHutQuest(w, quest, reward);
				}
				w.skipZero(2);
				break;
			}

			default:
				throw std::runtime_error("TinyH3MBuilder: object body not implemented for id="
					+ std::to_string(obj.id.getNum()));
		}
	}
}

void TinyH3MBuilder::writeHeroBody(TinyH3MWriter & w, const ObjectSpec & obj) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// Mirror of CMapLoaderH3M::readHero, non-HOTA branch. Each optional field
	// is gated on whether the corresponding spec field was populated by the
	// hero*() setter; absent fields write the "default" bool path so the engine
	// uses its standard hero loadout.
	if(features.levelAB)
		w.writeUInt32(obj.wireIdentifier); // wire identifier (referenced by kill-hero quests)
	w.writePlayer(obj.owner);
	w.writeHero(obj.heroType);

	w.writeBool(false);                    // hasName (always default for now)

	const bool hasExp = obj.heroExperienceXp.has_value();
	if(features.levelSOD)
	{
		w.writeBool(hasExp);
		if(hasExp)
			w.writeUInt32(*obj.heroExperienceXp);
	}
	else
	{
		w.writeUInt32(hasExp ? *obj.heroExperienceXp : 0); // <=AB: always-present field; 0 = unset
	}

	w.writeBool(false);                    // hasPortrait
	const bool hasSecSkills = !obj.heroSecondarySkills.empty();
	w.writeBool(hasSecSkills);
	if(hasSecSkills)
	{
		w.writeUInt32(static_cast<uint32_t>(obj.heroSecondarySkills.size()));
		for(const auto & [skill, level] : obj.heroSecondarySkills)
		{
			assert(skill.hasValue());
			assert(level >= 1 && level <= 3);
			w.writeUInt8(static_cast<uint8_t>(skill.getNum()));
			w.writeUInt8(level);
		}
	}

	const bool hasGarison = !obj.heroGarrisonStacks.empty();
	w.writeBool(hasGarison);
	if(hasGarison)
		writeCreatureSet(w, obj.heroGarrisonStacks);

	w.writeInt8(0);                        // formation = LOOSE

	const bool hasArtSet = !obj.heroEquippedArts.empty() || !obj.heroBackpackArts.empty();
	w.writeBool(hasArtSet);
	if(hasArtSet)
		writeArtifactSet(w, obj.heroEquippedArts, obj.heroBackpackArts);

	w.writeUInt8(0xff);                    // patrolRadius — 0xff disables patrol

	if(features.levelAB)
	{
		w.writeBool(false);                // hasCustomBiography
		w.writeInt8(-1);                   // gender = DEFAULT
	}

	if(features.levelSOD)
	{
		const bool hasCustomSpells = obj.heroSpells.has_value();
		w.writeBool(hasCustomSpells);
		if(hasCustomSpells)
			writeSpellBitmask(w, *obj.heroSpells);

		const bool hasPrim = obj.heroPrimarySkills.has_value();
		w.writeBool(hasPrim);
		if(hasPrim)
		{
			for(int i = 0; i < 4; ++i)
				w.writeUInt8((*obj.heroPrimarySkills)[i]);
		}
	}
	w.skipZero(16);
}

void TinyH3MBuilder::writeScrollBody(TinyH3MWriter & w, const ObjectSpec & obj) const
{
	// Mirror of CMapLoaderH3M::readScroll: readMessageAndGuards (hasMessage=false
	// is a single zero byte) + 4-byte spell id.
	w.writeBool(false);                    // hasMessage
	w.writeSpell32(obj.scrollSpell);
}

void TinyH3MBuilder::writeSpellBitmask(TinyH3MWriter & w, const std::vector<SpellID> & spells) const
{
	const auto features = MapFormatFeaturesH3M::find(format, /*hotaVersion*/ 0);
	std::vector<std::byte> mask(static_cast<size_t>(features.spellsBytes));

	for(const auto & spell : spells)
	{
		assert(spell.hasValue());
		assert(spell.getNum() < features.spellsCount);
		const auto index = static_cast<size_t>(spell.getNum());
		mask.at(index / 8) |= static_cast<std::byte>(1 << (index % 8));
	}

	for(const auto byte : mask)
		w.writeUInt8(std::to_integer<uint8_t>(byte));
}

void TinyH3MBuilder::writeCreatureSet(TinyH3MWriter & w, const std::vector<std::pair<CreatureID, uint16_t>> & stacks) const
{
	constexpr int slotCount = 7;
	for(int i = 0; i < slotCount; ++i)
	{
		if(i < static_cast<int>(stacks.size()))
		{
			w.writeCreature(stacks[i].first);
			w.writeUInt16(stacks[i].second);
		}
		else
		{
			w.writeCreature(CreatureID::NONE);
			w.writeUInt16(0);
		}
	}
}

void TinyH3MBuilder::writeArtifactSet(TinyH3MWriter & w,
	const std::vector<std::pair<ArtifactPosition, ArtifactID>> & equipped,
	const std::vector<ArtifactID> & backpack) const
{
	auto features = featuresFor(format, hotaFormatVersion);

	// Build a slot lookup so we can iterate equipped slots in fixed order
	// regardless of the user's input order.
	std::map<int, ArtifactID> bySlot;
	for(const auto & entry : equipped)
		bySlot[entry.first.getNum()] = entry.second;

	for(int slot = 0; slot < features.artifactSlotsCount; ++slot)
	{
		auto it = bySlot.find(slot);
		w.writeArtifact(it == bySlot.end() ? ArtifactID(ArtifactID::NONE) : it->second);
	}
	w.writeUInt16(static_cast<uint16_t>(backpack.size()));
	for(ArtifactID art : backpack)
		w.writeArtifact(art);
}

void TinyH3MBuilder::writeQuestBody(TinyH3MWriter & w, const Quest & quest) const
{
	// readQuest reads a single int8 missionId then dispatches per-type. NONE
	// returns immediately; everything else falls through to lastDay + 3 strings.
	if(quest.kind == EQuestMission::NONE)
	{
		w.writeInt8(static_cast<int8_t>(EQuestMission::NONE));
		return;
	}

	// HotA missions ride on the HOTA_MULTI placeholder byte (10) + a uint32 subID;
	// every other kind writes its own missionId byte directly.
	const bool isHotaMission = quest.kind == EQuestMission::HOTA_HERO_CLASS
		|| quest.kind == EQuestMission::HOTA_REACH_DATE
		|| quest.kind == EQuestMission::HOTA_GAME_DIFFICULTY
		|| quest.kind == EQuestMission::HOTA_SCRIPTED;
	w.writeInt8(static_cast<int8_t>(isHotaMission ? EQuestMission::HOTA_MULTI_PLACEHOLDER : quest.kind));

	switch(quest.kind)
	{
		case EQuestMission::PRIMARY_SKILL:
			for(int i = 0; i < 4; ++i)
				w.writeUInt8(quest.primarySkills[i]);
			break;

		case EQuestMission::LEVEL:
			w.writeUInt32(quest.heroLevel);
			break;

		case EQuestMission::ARTIFACT:
			w.writeUInt8(static_cast<uint8_t>(quest.artifacts.size()));
			for(ArtifactID art : quest.artifacts)
				w.writeArtifact(art);          // 1 byte ROE, 2 bytes AB+
			// HOTA5+ trailing scroll spell not emitted (builder is non-HOTA).
			break;

		case EQuestMission::ARMY:
			w.writeUInt8(static_cast<uint8_t>(quest.creatures.size()));
			for(const auto & stack : quest.creatures)
			{
				w.writeCreature(stack.first);  // 1 byte ROE, 2 bytes AB+
				w.writeUInt16(stack.second);
			}
			break;

		case EQuestMission::RESOURCES:
			for(int i = 0; i < 7; ++i)
				w.writeUInt32(quest.resources[i]);
			break;

		case EQuestMission::HERO:
			w.writeHero(quest.hero);
			break;

		case EQuestMission::PLAYER:
			w.writePlayer(quest.player);
			break;

		case EQuestMission::KILL_CREATURE:
		case EQuestMission::KILL_HERO:
			// uint32 wire identifier of the target object (set by the builder when
			// the monster / hero was added; surfaced via .lastHandle()).
			w.writeUInt32(quest.killTargetIdentifier);
			break;

		case EQuestMission::HOTA_HERO_CLASS:
		{
			w.writeUInt32(0); // missionSubID
			// sized hero-class bitmask: uint32 count + ceil(count/8) bytes, bit i => class i
			uint32_t classBytes = 0;
			for(HeroClassID hc : quest.heroClasses)
				classBytes = std::max(classBytes, static_cast<uint32_t>(hc.getNum()) / 8 + 1);
			std::vector<uint8_t> mask(classBytes, 0);
			for(HeroClassID hc : quest.heroClasses)
				mask[hc.getNum() / 8] |= static_cast<uint8_t>(1u << (hc.getNum() % 8));
			w.writeUInt32(classBytes * 8);
			for(uint8_t b : mask)
				w.writeUInt8(b);
			break;
		}

		case EQuestMission::HOTA_REACH_DATE:
			w.writeUInt32(1);                      // missionSubID
			w.writeUInt32(quest.reachDateDay - 1); // loader stores wire value + 1
			break;

		case EQuestMission::HOTA_GAME_DIFFICULTY:
			w.writeUInt32(2);                      // missionSubID
			w.writeUInt32(quest.difficultyMask);
			break;

		case EQuestMission::HOTA_SCRIPTED:
			w.writeUInt32(3);                      // missionSubID
			w.writeUInt32(quest.scriptEventID);
			w.writeBool(false);                    // unknown trailing bool, meaning TBD
			break;

		default:
			throw std::runtime_error("TinyH3MBuilder: mission kind "
				+ std::to_string(static_cast<int>(quest.kind)) + " not implemented");
	}

	w.writeInt32(quest.lastDay);
	w.writeBaseString(quest.firstVisitText);
	w.writeBaseString(quest.nextVisitText);
	w.writeBaseString(quest.completedText);
}

void TinyH3MBuilder::writeRewardBody(TinyH3MWriter & w, const SeerReward & reward) const
{
	w.writeInt8(static_cast<int8_t>(reward.kind));
	switch(reward.kind)
	{
		case SeerReward::Kind::NOTHING:
			break;
		case SeerReward::Kind::EXPERIENCE:
		case SeerReward::Kind::MANA:
			w.writeUInt32(reward.amount);
			break;
		case SeerReward::Kind::RESOURCES:
			w.writeGameResID(reward.resourceType);
			w.writeUInt32(reward.resourceAmount);
			break;
	}
}

void TinyH3MBuilder::writeSeerHutQuest(TinyH3MWriter & w, const Quest & quest, const SeerReward & reward) const
{
	// readSeerHutQuest reads the mission, then a reward block only when the mission
	// is non-NONE. NONE keeps a single zero placeholder byte (matches the existing
	// single-quest fixtures; NONE-mission seer huts are not used in practice).
	writeQuestBody(w, quest);
	if(quest.kind != EQuestMission::NONE)
		writeRewardBody(w, reward);
	else
		w.skipZero(1);
}

void TinyH3MBuilder::writeEvents(TinyH3MWriter & w) const
{
	// uint32 count + per-event body. No global events.
	w.writeUInt32(0);
}

} // namespace TinyH3M
