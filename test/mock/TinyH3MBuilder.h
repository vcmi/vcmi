/*
 * TinyH3MBuilder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/GameConstants.h"
#include "../../lib/int3.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/mapping/MapDifficulty.h"

namespace TinyH3M
{

class TinyH3MWriter;

/// Stable wire identifier of a placed object. Returned by .monster / .hero /
/// .randomHero so a later .missionKillCreature(handle) / .missionKillHero(handle)
/// can reference the right object regardless of the order objects are appended.
struct ObjectHandle
{
	uint32_t wireIdentifier = 0;
};

/// Quest mission spec consumed by .questGuard(...) / .seerHut(...). Construct via
/// the Mission* factories below; only fields appropriate to `kind` are read at
/// emit time.
struct Quest
{
	EQuestMission                                kind = EQuestMission::NONE;
	std::vector<ArtifactID>                      artifacts;       // ARTIFACT
	std::vector<std::pair<CreatureID, uint16_t>> creatures;       // ARMY (creature + count)
	std::array<uint32_t, 7>                      resources{};     // RESOURCES (per-resource amount)
	std::array<uint8_t, 4>                       primarySkills{}; // PRIMARY_SKILL (attack, defense, spell power, knowledge)
	uint32_t                                     heroLevel = 0;   // LEVEL
	HeroTypeID                                   hero;            // HERO
	PlayerColor                                  player = PlayerColor::NEUTRAL; // PLAYER
	uint32_t                                     killTargetIdentifier = 0; // KILL_CREATURE / KILL_HERO (wire identifier of target)

	/// Day-of-game past which the quest can no longer be completed. -1 (default) = no timeout.
	int32_t                                      lastDay = -1;

	/// Localized texts displayed on first / repeat / completion visits. Empty
	/// strings mean "use engine default text"; non-empty values are written to
	/// the wire and resolved by the loader through mapRegisterLocalizedString.
	std::string                                  firstVisitText;
	std::string                                  nextVisitText;
	std::string                                  completedText;

	// Fluent setters so a Mission* factory result can be augmented inline:
	//   B::missionResources({...}).withLastDay(7).withFirstVisitText("Bring me...")
	Quest & withLastDay(int32_t day)                  { lastDay        = day;             return *this; }
	Quest & withFirstVisitText(std::string text)      { firstVisitText = std::move(text); return *this; }
	Quest & withNextVisitText(std::string text)       { nextVisitText  = std::move(text); return *this; }
	Quest & withCompletedText(std::string text)       { completedText  = std::move(text); return *this; }
};

/// Seer hut reward. NONE-mission seer huts do not read a reward, so RewardKind
/// is only consulted when the mission is non-NONE.
struct SeerReward
{
	// Underlying byte values match ESeerHutRewardType in MapFormatH3M.cpp.
	enum class Kind : uint8_t
	{
		NOTHING    = 0,
		EXPERIENCE = 1,
		MANA       = 2,
		RESOURCES  = 5,
	};

	Kind      kind           = Kind::NOTHING;
	uint32_t  amount         = 0;             // EXPERIENCE, MANA
	GameResID resourceType;                   // RESOURCES
	uint32_t  resourceAmount = 0;             // RESOURCES
};

/// Fluent API for building .h3m binary streams in test code.
///
/// Builder holds intent-level state. At build() time, per-section emitters
/// walk the state and produce bytes via TinyH3MWriter, consulting
/// MapFormatFeaturesH3M for per-format field sizes.
///
/// Defaults produce a parseable, empty, editor-openable map. Fluent calls
/// add players and objects on top.
class TinyH3MBuilder
{
public:
	explicit TinyH3MBuilder(EMapFormat format = EMapFormat::ROE);

	// ---- fluent setters -------------------------------------------------

	TinyH3MBuilder & size(int sideLength, bool twoLevel = false);
	TinyH3MBuilder & name(std::string s);
	TinyH3MBuilder & description(std::string s);
	TinyH3MBuilder & difficulty(EMapDifficulty d);

	/// Mark a player as both human- and computer-playable. Required for any
	/// object that ownership-validates against canAnyonePlay (towns, heroes).
	TinyH3MBuilder & playerActive(PlayerColor color);

	/// Append a Random Town object owned by `owner`. Builder auto-registers the
	/// RANDOM_TOWN template on first call. No garrison, standard fort, no events.
	TinyH3MBuilder & randomTown(const int3 & pos, PlayerColor owner);

	// ---- heroes --------------------------------------------------------

	/// Place a fixed hero of the given type. All optional fields (name, experience,
	/// secondary skills, garrison, biography, custom spells, primary skills, ...)
	/// are emitted as "default" unless the matching `hero*(...)` setter below is
	/// chained immediately after.
	TinyH3MBuilder & hero(const int3 & pos, HeroTypeID type, PlayerColor owner);

	/// Place a random hero owned by `owner`. Type is resolved at game start.
	TinyH3MBuilder & randomHero(const int3 & pos, PlayerColor owner);

	// ---- hero customisation (apply to the most recently added hero) -----
	// Each setter asserts the last object is a HERO / RANDOM_HERO.

	/// Up to 7 stacks. Stacks beyond the 7th are dropped on the floor — the
	/// wire format has exactly 7 slots.
	TinyH3MBuilder & heroGarrison(std::vector<std::pair<CreatureID, uint16_t>> stacks);

	/// Total experience points (the engine derives hero level from this).
	TinyH3MBuilder & heroExperience(uint32_t totalXp);

	/// Per-skill primary stat overrides (attack, defense, spell power, knowledge).
	TinyH3MBuilder & heroPrimary(uint8_t attack, uint8_t defense, uint8_t spellPower, uint8_t knowledge);

	/// Equipped artifacts keyed by slot. Slots not listed are written as NONE
	/// (empty). Triggers the hasArtSet branch in `loadArtifactsOfHero`.
	TinyH3MBuilder & heroEquipped(std::vector<std::pair<ArtifactPosition, ArtifactID>> equipped);

	/// Backpack artifacts. Order matches read order. Triggers hasArtSet even
	/// when no equipped slots are populated.
	TinyH3MBuilder & heroBackpack(std::vector<ArtifactID> backpack);

	// ---- wanderer objects ----------------------------------------------

	/// Monster stack on the adventure map. `character` is 0..4 (compliant..savage).
	TinyH3MBuilder & monster(const int3 & pos, CreatureID creature, uint16_t count, int8_t character = 2);

	/// Resource pile. amount=0 means "use default".
	TinyH3MBuilder & resource(const int3 & pos, GameResID resource, uint32_t amount = 0);

	/// Specific artifact pickup.
	TinyH3MBuilder & artifact(const int3 & pos, ArtifactID artifact);

	/// Spell scroll pickup containing the given spell.
	TinyH3MBuilder & scroll(const int3 & pos, SpellID spell);

	// ---- quest objects -------------------------------------------------

	/// Keymaster Tent. Subid encodes the keymaster colour (0..7).
	TinyH3MBuilder & keymaster(const int3 & pos, int color);

	/// Border Guard checking for the matching keymaster colour.
	TinyH3MBuilder & borderGuard(const int3 & pos, int color);

	/// Border Gate (pathfinder-passable variant) checking for the matching keymaster colour.
	TinyH3MBuilder & borderGate(const int3 & pos, int color);

	/// Quest Guard. With the default-constructed Quest{} the mission is NONE
	/// (always accepts); pass one of the Mission* factories to require a hero to
	/// satisfy the mission before passing.
	TinyH3MBuilder & questGuard(const int3 & pos, Quest mission = {});

	/// Seer Hut. With default-constructed Quest{} no mission is required and the
	/// reward is ignored. With a real mission, the reward kind controls what the
	/// hero receives on completion.
	TinyH3MBuilder & seerHut(const int3 & pos, Quest mission = {}, SeerReward reward = {});

	// ---- mission factories (for .questGuard / .seerHut) -----------------

	static Quest missionArtifacts(std::vector<ArtifactID> artifacts);
	static Quest missionArmy(std::vector<std::pair<CreatureID, uint16_t>> stacks);
	static Quest missionResources(std::array<uint32_t, 7> amounts);
	static Quest missionPrimarySkills(uint8_t attack, uint8_t defense, uint8_t spellPower, uint8_t knowledge);
	static Quest missionLevel(uint32_t level);
	static Quest missionHero(HeroTypeID hero);
	static Quest missionPlayer(PlayerColor player);
	static Quest missionKillCreature(ObjectHandle target);
	static Quest missionKillHero(ObjectHandle target);

	// ---- seer-hut reward factories --------------------------------------

	static SeerReward rewardNothing();
	static SeerReward rewardExperience(uint32_t amount);
	static SeerReward rewardMana(uint32_t amount);
	static SeerReward rewardResource(GameResID resource, uint32_t amount);

	// ---- output ---------------------------------------------------------

	/// Emit the uncompressed .h3m byte stream. The output covers everything
	/// CMapLoaderH3M::loadMapHeader needs to parse and nothing else.
	std::vector<uint8_t> build();

	/// Build the uncompressed bytes, gzip them, and drop the resulting .h3m
	/// into VCMIDirs::get().userCachePath() / "testMaps" / (testName + ".h3m").
	/// Returns the same uncompressed bytes (so callers can immediately feed
	/// them to CMapLoaderH3M without re-decompressing). Intended to be called
	/// from every .h3m-producing test so the fixtures can be opened in the
	/// original H3 / HOTA editor for manual verification.
	std::vector<uint8_t> buildAndDump(const std::string & testName);

	/// Wire identifier of the most recently added object. Use in conjunction
	/// with `missionKillCreature` / `missionKillHero` to point a quest at an
	/// earlier monster or hero. Asserts there is at least one added object.
	ObjectHandle lastHandle() const;

private:
	struct ObjectSpec
	{
		MapObjectID    id;
		MapObjectSubID subid;
		int3           position;
		PlayerColor    owner = PlayerColor::NEUTRAL;
		uint32_t       templateIndex = 0; // resolved at build() time

		// Body-specific payload. Only the fields appropriate for `id` are read.
		uint16_t       monsterCount      = 0;
		int8_t         monsterCharacter  = 0;
		uint32_t       resourceAmount    = 0;
		HeroTypeID     heroType;            // HERO / RANDOM_HERO
		SpellID        scrollSpell;         // SPELL_SCROLL
		Quest          quest;               // QUEST_GUARD / SEER_HUT
		SeerReward     reward;              // SEER_HUT (only consumed when quest is non-NONE)

		// Hero customisation. Only consumed for HERO / RANDOM_HERO objects.
		std::vector<std::pair<CreatureID, uint16_t>>           heroGarrisonStacks;
		std::optional<uint32_t>                                heroExperienceXp;
		std::optional<std::array<uint8_t, 4>>                  heroPrimarySkills;
		std::vector<std::pair<ArtifactPosition, ArtifactID>>   heroEquippedArts;
		std::vector<ArtifactID>                                heroBackpackArts;

		// Wire identifier emitted on AB+ for heroes / monsters / objects, exposed
		// via ObjectHandle so later mission factories can target this object.
		// Zero means "no one cares about this object" — assigned at registration
		// time as a stable sequence so kill-quest references survive append-only
		// edits to the object list.
		uint32_t       wireIdentifier = 0;
	};

	// Internal helper: register a (id, subid) template in the table if not already there;
	// return its index. The actual byte content is sourced from Data/Objects.txt at emit time.
	uint32_t registerTemplate(MapObjectID id, MapObjectSubID subid);

	void writeHeader(TinyH3MWriter & w) const;
	void writePlayerInfo(TinyH3MWriter & w) const;
	void writeStandardVictoryLoss(TinyH3MWriter & w) const;
	void writeTeamInfo(TinyH3MWriter & w) const;
	void writeAllowedHeroes(TinyH3MWriter & w) const;
	void writeDisposedHeroes(TinyH3MWriter & w) const;
	void writeMapOptions(TinyH3MWriter & w) const;
	void writeAllowedArtifacts(TinyH3MWriter & w) const;
	void writeAllowedSpellsAbilities(TinyH3MWriter & w) const;
	void writeRumors(TinyH3MWriter & w) const;
	void writePredefinedHeroes(TinyH3MWriter & w) const;
	void writeTerrain(TinyH3MWriter & w) const;
	void writeObjectTemplates(TinyH3MWriter & w) const;
	void writeObjects(TinyH3MWriter & w) const;
	void writeEvents(TinyH3MWriter & w) const;

	void writeHeroBody(TinyH3MWriter & w, const ObjectSpec & obj) const;
	void writeScrollBody(TinyH3MWriter & w, const ObjectSpec & obj) const;
	/// Mirrors readCreatureSet: 7 fixed slots, each = creature id + uint16 count.
	/// Slots past the end of `stacks` are written as NONE / 0.
	void writeCreatureSet(TinyH3MWriter & w, const std::vector<std::pair<CreatureID, uint16_t>> & stacks) const;
	/// Mirrors loadArtifactsOfHero body (after hasArtSet=true): features.artifactSlotsCount
	/// equipped slots in fixed order + uint16 backpack count + backpack entries.
	void writeArtifactSet(TinyH3MWriter & w,
		const std::vector<std::pair<ArtifactPosition, ArtifactID>> & equipped,
		const std::vector<ArtifactID> & backpack) const;
	/// Emits the missionId byte, mission-type-specific payload, lastDay and the
	/// three localized-text strings. NONE missions emit only the missionId byte.
	void writeQuestBody(TinyH3MWriter & w, const Quest & quest) const;
	/// Emits 1 byte rewardKind + payload. Caller must only invoke when the
	/// preceding mission was non-NONE — NONE missions skip the reward block.
	void writeRewardBody(TinyH3MWriter & w, const SeerReward & reward) const;

	EMapFormat     format;
	int            sideLength = 36;
	bool           twoLevel = false;
	std::string    mapName = "TinyH3M test map";
	std::string    mapDescription;
	EMapDifficulty mapDifficulty = EMapDifficulty::NORMAL;

	std::array<bool, 8> playerEnabled{};
	std::vector<std::pair<MapObjectID, MapObjectSubID>> templates;
	std::vector<ObjectSpec>                              objects;
	uint32_t                                             nextWireIdentifier = 1;

	void registerObject(ObjectSpec spec);
	ObjectSpec & lastObject(); // asserts there is one
};

} // namespace TinyH3M
