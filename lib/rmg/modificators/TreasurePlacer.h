/*
 * TreasurePlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../ObjectInfo.h"
#include "../Zone.h"
#include "../CMapGenerator.h"
#include "../../mapObjects/ObjectTemplate.h"
#include "../../rewardable/Configuration.h"

#include <vstd/RNG.h>

class CGObjectInstance;
class CGPandoraBox;
class IGameInfoCallback;
class ObjectManager;
class QuestArtifactPlacer;
class RmgMap;
class CMapGenerator;
class SeerHut;
struct RandomMapInfo;

class TreasurePlacer: public Modificator
{
public:
	MODIFICATOR(TreasurePlacer);

	/// Built-in gold and experience boxes hold i * PANDORA_AMOUNT_STEP; randomMap.json pandora multipliers are the value of one step
	static constexpr ui32 PANDORA_AMOUNT_STEP = 5000;
	static constexpr ui16 PANDORA_GOLD_RARITY = 5;
	static constexpr ui16 PANDORA_EXPERIENCE_RARITY = 20;
	static constexpr ui16 PANDORA_CREATURES_RARITY = 3;
	static constexpr ui16 PANDORA_SPELLS_RARITY = 2;
	static constexpr ui16 SEER_HUT_GOLD_RARITY = 10;
	static constexpr ui16 SEER_HUT_EXPERIENCE_RARITY = 10;
	static constexpr ui16 SEER_HUT_CREATURES_RARITY = 3;

	enum class NativeCreatureKind { ALL, BASIC, UPGRADE };

	/// Defaults of one customObjects reward list; gold and experience value = amount * multiplier / amountStep
	struct CustomRewardDefaults
	{
		std::string listName; ///< "pandoraBoxes" or "seerHuts", for log messages
		ui32 amountStep = 1;
		int goldMultiplier = 1;
		int experienceMultiplier = 1;
		ui16 goldRarity = 0;
		ui16 experienceRarity = 0;
		ui16 creaturesRarity = 0;
	};
	static CustomRewardDefaults pandoraRewardDefaults(const CMapGenerator::Config & config);
	static CustomRewardDefaults seerHutRewardDefaults();

	/// "rmg" block of a customObjects.pandoraBoxes or seerHuts entry
	struct CustomReward
	{
		std::optional<ui32> value; ///< nullopt only for native creature boxes without rmg.value: valued like the built-in creature box
		ui16 rarity = 0;
		ui32 zoneLimit = 0; ///< 0 = unlimited
		std::set<FactionID> zoneFactions; ///< empty = all
		std::optional<int> nativeCreaturesLevel; ///< set for rmg.nativeCreatures boxes; 0 = all levels
		NativeCreatureKind nativeCreaturesKind = NativeCreatureKind::ALL; ///< ALL unless nativeCreaturesLevel >= 1
	};
	/// rmg fields of an entry with preset defaults applied; nullopt (and an error log) when value or rarity is missing
	static std::optional<CustomReward> readCustomReward(const JsonNode & entry, const CustomRewardDefaults & defaults);
	/// natives that get an rmg.nativeCreatures box, in natives order. An upgrade is a creature that isUpgradeOf another creature in natives; basic is every other creature
	static std::vector<const CCreature *> selectNativeCreatures(const CustomReward & reward, const std::vector<const CCreature *> & natives);
	/// the entry without "rmg", loaded as Pandora box map options exactly like the .vmap loader does
	static std::shared_ptr<CGPandoraBox> loadPandoraBoxOptions(const JsonNode & entry, IGameInfoCallback * cb);
	/// the single reward of an entry's content; nullopt (and an error log) unless it loads to exactly one
	static std::optional<Rewardable::VisitInfo> readSingleReward(const JsonNode & entry, IGameInfoCallback * cb);
	/// gives the hut its one quest: bring artifact, receive reward (stored in Quest::reward)
	static void addSeerHutQuest(SeerHut & hut, ArtifactID artifact, const Rewardable::VisitInfo & reward);
	/// candidate indices for slots, weighted by probability, each candidate at most maxPerZone times; stops early when every candidate is used up
	static std::vector<size_t> pickSeerHutSlots(const std::vector<ObjectInfo> & candidates, size_t slots, vstd::RNG & rng);

	void process() override;
	void init() override;
	char dump(const int3 &) override;
	
	void createTreasures(ObjectManager & manager);
	void addObjectToRandomPool(const ObjectInfo& oi);
	void setBasicProperties(ObjectInfo & oi, CompoundMapObjectID objid) const;
	/// fills value and probability left at 0 from the object type's rmg defaults; false when either has no default (0)
	static bool applyCommonObjectDefaults(ObjectInfo & object, const RandomMapInfo & defaults);
	/// for randomDwellingLvl, a 0 value becomes the native dwelling value and a 0 probability becomes NATIVE_DWELLING_RARITY
	void setRandomDwellingDefaults(ObjectInfo & oi) const;

	// TODO: Can be defaulted to addAllPossibleObjects, but then each object will need to be configured
	void addCommonObjects();
	void addDwellings();
	void addPandoraBoxes();
	void addPandoraBoxesWithGold();
	void addPandoraBoxesWithExperience();
	void addPandoraBoxesWithCreatures();
	/// Built-in creature box content and value for a native creature; nullopt when the creature gets no box
	std::optional<ObjectInfo> createCreaturePandoraBox(const CCreature * creature);
	void addPandoraBoxesWithSpells();
	/// Boxes from customObjects.pandoraBoxes; added after zone bans are applied, so bans never remove them
	void addCustomPandoraBoxes();
	/// Scrolls from customObjects.spellScrolls; added after zone bans are applied, so bans never remove them
	void addCustomSpellScrolls();
	std::shared_ptr<CGPandoraBox> createPandoraBox() const;
	void addSeerHuts();
	ObjectInfo createSeerHut(const Rewardable::VisitInfo & reward, ui32 rewardValue, int appearance, QuestArtifactPlacer & qap);
	std::optional<ObjectInfo> createCreatureSeerHut(const CCreature * creature, int appearance, QuestArtifactPlacer & qap);
	void addCustomSeerHuts();
	void addPrisons();
	void addScrolls();
	void addAllPossibleObjects(); //add objects, including zone-specific, to possibleObjects
	// TODO: Read custom object config from zone file

	/// Get all objects for this terrain

	void setMaxPrisons(size_t count);
	size_t getMaxPrisons() const;
	int creatureToCount(const CCreature * creature) const;
	
protected:
	bool isGuardNeededForTreasure(int value);
	
	ObjectInfo * getRandomObject(ui32 desiredValue, ui32 currentValue, bool allowLargeObjects);
	/// templates may belong to another object type than their ObjectInfo, e.g. seer huts with random appearance
	static bool isVisitableFromTop(const std::vector<std::shared_ptr<const ObjectTemplate>> & templates);
	std::vector<ObjectInfo*> prepareTreasurePile(const CTreasureInfo & treasureInfo);
	rmg::Object constructTreasurePile(const std::vector<ObjectInfo*> & treasureInfos, bool densePlacement = false);

protected:
	class ObjectPool
	{
	public:
		void addObject(const ObjectInfo & info);
		void updateObject(MapObjectID id, MapObjectSubID subid, ObjectInfo info);
		std::vector<ObjectInfo> & getPossibleObjects();
		void patchWithZoneConfig(const Zone & zone, TreasurePlacer * tp);
		void sortPossibleObjects();
		void discardObjectsAboveValue(ui32 value);

	private:

		std::vector<ObjectInfo> possibleObjects;
		std::map<CompoundMapObjectID, ObjectInfo> customObjects;

	} objects;
	// TODO: Need to nagivate and update these

	int minGuardedValue = 0;
	
	rmg::Area treasureArea;
	rmg::Area treasureBlockArea;
	rmg::Area guards;

	size_t maxPrisons;

	std::vector<const CCreature *> creatures; //native creatures for this zone
	std::vector<int> tierValues;
	std::vector<ObjectInfo> customSeerHuts; // filled by addSeerHuts, so bans cannot remove them
};
