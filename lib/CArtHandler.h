/*
 * CArtHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Artifact.h>
#include <vcmi/ArtifactService.h>

#include "bonuses/Bonus.h"
#include "bonuses/CBonusSystemNode.h"
#include "IHandlerBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtHandler;
class CGHeroInstance;
class CMap;
class CGameState;
class CArtifactSet;
class CArtifactInstance;
class JsonSerializeFormat;

#define ART_BEARER_LIST \
	ART_BEARER(HERO)\
	ART_BEARER(CREATURE)\
	ART_BEARER(COMMANDER)\
	ART_BEARER(ALTAR)

namespace ArtBearer
{
	enum ArtBearer
	{
#define ART_BEARER(x) x,
		ART_BEARER_LIST
#undef ART_BEARER
	};
}

class DLL_LINKAGE CCombinedArtifact
{
protected:
	CCombinedArtifact() : fused(false) {};

	std::vector<const CArtifact*> constituents; // Artifacts IDs a combined artifact consists of, or nullptr.
	std::set<const CArtifact*> partOf; // Reverse map of constituents - combined arts that include this art
	bool fused;

public:
	bool isCombined() const;
	const std::vector<const CArtifact*> & getConstituents() const;
	const std::set<const CArtifact*> & getPartOf() const;
	void setFused(bool isFused);
	bool isFused() const;
	bool hasParts() const;
};

class DLL_LINKAGE CScrollArtifact
{
protected:
	CScrollArtifact() = default;
public:
	bool isScroll() const;
};

class DLL_LINKAGE CGrowingArtifact
{
protected:
	CGrowingArtifact() = default;

	std::vector <std::pair<ui16, Bonus>> bonusesPerLevel; // Bonus given each n levels
	std::vector <std::pair<ui16, Bonus>> thresholdBonuses; // After certain level they will be added once
public:
	bool isGrowing() const;

	std::vector <std::pair<ui16, Bonus>> & getBonusesPerLevel();
	const std::vector <std::pair<ui16, Bonus>> & getBonusesPerLevel() const;
	std::vector <std::pair<ui16, Bonus>> & getThresholdBonuses();
	const std::vector <std::pair<ui16, Bonus>> & getThresholdBonuses() const;
};

// Container for artifacts. Not for instances.
class DLL_LINKAGE CArtifact
	: public Artifact, public CBonusSystemNode, public CCombinedArtifact, public CScrollArtifact, public CGrowingArtifact
{
	ArtifactID id;
	std::string image;
	std::string large; // big image for custom artifacts, used in drag & drop
	std::string advMapDef; // used for adventure map object
	std::string modScope;
	std::string identifier;
	int32_t iconIndex;
	uint32_t price;
	CreatureID warMachine;
	// Bearer Type => ids of slots where artifact can be placed
	std::map<ArtBearer::ArtBearer, std::vector<ArtifactPosition>> possibleSlots;

public:
	enum EartClass {ART_SPECIAL=1, ART_TREASURE=2, ART_MINOR=4, ART_MAJOR=8, ART_RELIC=16}; //artifact classes

	EartClass aClass = ART_SPECIAL;
	bool onlyOnWaterMap;

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override;
	ArtifactID getId() const override;
	const IBonusBearer * getBonusBearer() const override;

	std::string getDescriptionTranslated() const override;
	std::string getEventTranslated() const override;
	std::string getNameTranslated() const override;

	std::string getDescriptionTextID() const override;
	std::string getEventTextID() const override;
	std::string getNameTextID() const override;

	uint32_t getPrice() const override;
	CreatureID getWarMachine() const override;
	bool isBig() const override;
	bool isTradable() const override;

	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other
	std::string nodeName() const override;
	void addNewBonus(const std::shared_ptr<Bonus>& b) override;
	const std::map<ArtBearer::ArtBearer, std::vector<ArtifactPosition>> & getPossibleSlots() const;

	virtual bool canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot = ArtifactPosition::FIRST_AVAILABLE,
		bool assumeDestRemoved = false) const;
	void updateFrom(const JsonNode & data);
	// Is used for testing purposes only
	void setImage(int32_t iconIndex, std::string image, std::string large);

	CArtifact();
	~CArtifact();

	friend class CArtHandler;
};

class DLL_LINKAGE CArtHandler : public CHandlerBase<ArtifactID, Artifact, CArtifact, ArtifactService>
{
public:
	void addBonuses(CArtifact *art, const JsonNode &bonusList);

	static CArtifact::EartClass stringToClass(const std::string & className); //TODO: rework EartClass to make this a constructor

	bool legalArtifact(const ArtifactID & id) const;
	static void makeItCreatureArt(CArtifact * a, bool onlyCreature = true);
	static void makeItCommanderArt(CArtifact * a, bool onlyCommander = true);

	~CArtHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	std::set<ArtifactID> getDefaultAllowed() const;

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CArtifact> loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) override;

private:
	void addSlot(CArtifact * art, const std::string & slotID) const;
	void loadSlots(CArtifact * art, const JsonNode & node) const;
	void loadClass(CArtifact * art, const JsonNode & node) const;
	void loadType(CArtifact * art, const JsonNode & node) const;
	void loadComponents(CArtifact * art, const JsonNode & node);
};

VCMI_LIB_NAMESPACE_END
