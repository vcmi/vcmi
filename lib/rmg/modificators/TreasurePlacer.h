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
#include "../../mapObjects/ObjectTemplate.h"

class CGObjectInstance;
class ObjectManager;
class RmgMap;
class CMapGenerator;
struct RandomMapInfo;

class TreasurePlacer: public Modificator
{
public:
	MODIFICATOR(TreasurePlacer);
	
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
	/// Boxes from customObjects.pandoraBoxes; added after zone bans are applied, so bans never remove them
	void addCustomPandoraBoxes();
	/// Scrolls from customObjects.spellScrolls; added after zone bans are applied, so bans never remove them
	void addCustomSpellScrolls();
	/// Built-in huts go to the pool; custom huts are kept for addCustomSeerHuts
	void addSeerHuts();
	void addCustomSeerHuts();
	void addPrisons();
	void addScrolls();
	void addAllPossibleObjects(); //add objects, including zone-specific, to possibleObjects
	// TODO: Read custom object config from zone file

	/// Get all objects for this terrain

	void setMaxPrisons(size_t count);
	size_t getMaxPrisons() const;
	
protected:
	bool isGuardNeededForTreasure(int value);
	
	ObjectInfo * getRandomObject(ui32 desiredValue, ui32 currentValue, bool allowLargeObjects, const std::vector<ObjectInfo*> & pile);
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
	std::vector<ObjectInfo> customSeerHuts; // filled by addSeerHuts, so bans cannot remove them
};
