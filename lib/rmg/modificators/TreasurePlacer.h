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

class CGObjectInstance;
class CGPandoraBox;
class ObjectManager;
class RmgMap;
class CMapGenerator;
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

	enum class NativeCreatureKind { ALL, BASIC, UPGRADE };

	/// "rmg" block of a customObjects.pandoraBoxes entry
	struct CustomPandoraBox
	{
		std::optional<ui32> value; ///< nullopt only for native creature boxes without rmg.value: valued like the built-in creature box
		ui16 rarity = 0;
		ui32 zoneLimit = 0; ///< 0 = unlimited
		std::set<FactionID> zoneFactions; ///< empty = all
		std::optional<int> nativeCreaturesLevel; ///< set for rmg.nativeCreatures boxes; 0 = all levels
		NativeCreatureKind nativeCreaturesKind = NativeCreatureKind::ALL; ///< ALL unless nativeCreaturesLevel >= 1
	};
	/// rmg fields of a pandoraBoxes entry with preset defaults applied; nullopt (and an error log) when value or rarity is missing
	static std::optional<CustomPandoraBox> readCustomPandoraBox(const JsonNode & entry, const CMapGenerator::Config & config);
	/// natives that get an rmg.nativeCreatures box, in natives order. An upgrade is a creature that isUpgradeOf another creature in natives; basic is every other creature
	static std::vector<const CCreature *> selectNativeCreatures(const CustomPandoraBox & box, const std::vector<const CCreature *> & natives);

	void process() override;
	void init() override;
	char dump(const int3 &) override;
	std::string dumpLegend() const override;
	
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
	rmg::Area guards;

	size_t maxPrisons;

	std::vector<const CCreature *> creatures; //native creatures for this zone
	std::vector<int> tierValues;
};
