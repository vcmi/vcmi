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

	/// "rmg" block of a customObjects.pandoraBoxes entry
	struct CustomPandoraBox
	{
		std::optional<ui32> value; ///< nullopt only for native creature boxes without rmg.value: valued like the built-in creature box
		ui16 rarity = 0;
		ui32 zoneLimit = 0; ///< 0 = unlimited
		std::set<FactionID> zoneFactions; ///< empty = all
		std::optional<int> nativeCreaturesLevel; ///< set for rmg.nativeCreatures boxes; 0 = all levels
	};
	/// rmg fields of a pandoraBoxes entry with preset defaults applied; nullopt (and an error log) when value or rarity is missing
	static std::optional<CustomPandoraBox> readCustomPandoraBox(const JsonNode & entry, const CMapGenerator::Config & config);

	void process() override;
	void init() override;
	char dump(const int3 &) override;
	
	void createTreasures(ObjectManager & manager);
	void addObjectToRandomPool(const ObjectInfo& oi);
	void setBasicProperties(ObjectInfo & oi, CompoundMapObjectID objid) const;
	/// Values a random dwelling of a given level like a native dwelling of the zone's faction, unless the template set a value
	void setRandomDwellingValue(ObjectInfo & oi) const;

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
};
