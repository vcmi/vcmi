/*
 * CArtifact.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ArtBearer.h"
#include "EArtifactClass.h"

#include "../../bonuses/CBonusSystemNode.h"
#include "../../networkPacks/ArtifactLocation.h"

#include <vcmi/Artifact.h>

VCMI_LIB_NAMESPACE_BEGIN

class CArtifactSet;

class DLL_LINKAGE CCombinedArtifact
{
protected:
	CCombinedArtifact()
		: fused(false){};

	std::vector<const CArtifact *> constituents; // Artifacts IDs a combined artifact consists of, or nullptr.
	std::set<const CArtifact *> partOf; // Reverse map of constituents - combined arts that include this art
	bool fused;

public:
	bool isCombined() const;
	const std::vector<const CArtifact *> & getConstituents() const;
	const std::set<const CArtifact *> & getPartOf() const;
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

	std::vector<std::pair<ui16, std::shared_ptr<Bonus>>> bonusesPerLevel; // Bonus given each n levels
	std::vector<std::pair<ui16, std::shared_ptr<Bonus>>> thresholdBonuses; // After certain level they will be added once
public:
	bool isGrowing() const;

	std::vector<std::pair<ui16, std::shared_ptr<Bonus>>> & getBonusesPerLevel();
	const std::vector<std::pair<ui16, std::shared_ptr<Bonus>>> & getBonusesPerLevel() const;
	std::vector<std::pair<ui16, std::shared_ptr<Bonus>>> & getThresholdBonuses();
	const std::vector<std::pair<ui16, std::shared_ptr<Bonus>>> & getThresholdBonuses() const;
};

class DLL_LINKAGE CChargedArtifact
{
	DischargeArtifactCondition condition;
	bool removeOnDepletion;
	uint16_t defaultStartCharges;

protected:
	CChargedArtifact();

public:
	bool isCharged() const;

	void setCondition(const DischargeArtifactCondition & dischargeCondition);
	void setRemoveOnDepletion(const bool remove);
	void setDefaultStartCharges(const uint16_t charges);
	uint16_t getDefaultStartCharges() const;
	DischargeArtifactCondition getDischargeCondition() const;
	bool getRemoveOnDepletion() const;
};

// Container for artifacts. Not for instances.
class DLL_LINKAGE CArtifact final : public Artifact, public CBonusSystemNode,
		public CCombinedArtifact, public CScrollArtifact, public CGrowingArtifact, public CChargedArtifact
{
	ArtifactID id;
	std::string image;
	std::string advMapDef; // used for adventure map object
	std::string modScope;
	std::string identifier;
	int32_t iconIndex;
	uint32_t price;
	CreatureID warMachine;
	// Bearer Type => ids of slots where artifact can be placed
	std::map<ArtBearer, std::vector<ArtifactPosition>> possibleSlots;

public:
	/// Bonuses that are created for each instance of artifact
	std::vector<std::shared_ptr<Bonus>> instanceBonuses;

	std::string scenarioBonus;

	EArtifactClass aClass = EArtifactClass::ART_SPECIAL;
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
	std::string getBonusTextID(const std::string & bonusID) const;

	uint32_t getPrice() const override;
	CreatureID getWarMachine() const override;
	bool isBig() const override;
	bool isTradable() const override;

	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other
	std::string nodeName() const override;
	void addNewBonus(const std::shared_ptr<Bonus> & b) override;
	const std::map<ArtBearer, std::vector<ArtifactPosition>> & getPossibleSlots() const;

	virtual bool canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot = ArtifactPosition::FIRST_AVAILABLE, bool assumeDestRemoved = false) const;
	void updateFrom(const JsonNode & data);
	// Is used for testing purposes only
	void setImage(int32_t iconIndex, const std::string & image, const std::string & large);

	CArtifact();
	~CArtifact();

	friend class CArtHandler;
};

VCMI_LIB_NAMESPACE_END
