/*
 * CStackInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CStackBasicDescriptor.h"

#include "CCreatureHandler.h"
#include "bonuses/BonusCache.h"
#include "bonuses/CBonusSystemNode.h"
#include "callback/GameCallbackHolder.h"
#include "entities/artifact/CArtifactSet.h"
#include "mapObjects/CGObjectInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class CCreature;
class CGHeroInstance;
class CArmedInstance;
class CCreatureArtifactSet;
class JsonSerializeFormat;

class DLL_LINKAGE CStackInstance : public CBonusSystemNode, public CStackBasicDescriptor, public CArtifactSet, public ACreature, public GameCallbackHolder
{
	BonusValueCache nativeTerrain;
	BonusValueCache initiative;

	CArmedInstance * armyInstance = nullptr; //stack must be part of some army, army must be part of some object

	IGameInfoCallback * getCallback() const final
	{
		return cb;
	}

	TExpType totalExperience; //commander needs same amount of exp as hero
public:
	struct RandomStackInfo
	{
		uint8_t level;
		uint8_t upgrade;
	};
	// helper variable used during loading map, when object (hero or town) have creatures that must have same alignment.
	std::optional<RandomStackInfo> randomStack;

	CArmedInstance * getArmy();
	const CArmedInstance * getArmy() const; //stack must be part of some army, army must be part of some object
	void setArmy(CArmedInstance * ArmyObj);

	TExpType getTotalExperience() const;
	TExpType getAverageExperience() const;
	virtual bool canGainExperience() const;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & static_cast<CBonusSystemNode &>(*this);
		h & static_cast<CStackBasicDescriptor &>(*this);
		h & static_cast<CArtifactSet &>(*this);

		if(h.hasFeature(Handler::Version::STACK_INSTANCE_ARMY_FIX))
		{
			// no-op
		}
		if(h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			ObjectInstanceID dummyID;
			h & dummyID;
		}
		else
		{
			std::shared_ptr<CGObjectInstance> army;
			h & army;
		}

		h & totalExperience;
		if(!h.hasFeature(Handler::Version::STACK_INSTANCE_EXPERIENCE_FIX))
		{
			totalExperience *= getCount();
		}
	}

	void serializeJson(JsonSerializeFormat & handler);

	//overrides CBonusSystemNode
	std::string bonusToString(const std::shared_ptr<Bonus> & bonus) const override; // how would bonus description look for this particular type of node
	ImagePath bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const; //file name of graphics from StackSkills , in future possibly others

	//IConstBonusProvider
	const IBonusBearer * getBonusBearer() const override;
	//INativeTerrainProvider
	FactionID getFactionID() const override;

	virtual ui64 getPower() const;
	/// Returns total market value of resources needed to recruit this unit
	virtual ui64 getMarketValue() const;
	CCreature::CreatureQuantityId getQuantityID() const;
	std::string getQuantityTXT(bool capitalized = true) const;
	virtual int getExpRank() const;
	virtual int getLevel() const; //different for regular stack and commander
	CreatureID getCreatureID() const; //-1 if not available
	std::string getName() const; //plural or singular
	CStackInstance(IGameInfoCallback * cb);
	CStackInstance(IGameInfoCallback * cb, BonusNodeType nodeType, bool isHypothetic = false);
	CStackInstance(IGameInfoCallback * cb, const CreatureID & id, TQuantity count, bool isHypothetic = false);
	virtual ~CStackInstance() = default;

	void setType(const CreatureID & creID);
	void setType(const CCreature * c) final;
	void setCount(TQuantity amount) final;

	/// Gives specified amount of stack experience that will not be scaled by unit size
	void giveAverageStackExperience(TExpType exp);
	void giveTotalStackExperience(TExpType exp);

	bool valid(bool allowUnrandomized) const;
	ArtPlacementMap putArtifact(const ArtifactPosition & pos, const CArtifactInstance * art) override; //from CArtifactSet
	void removeArtifact(const ArtifactPosition & pos) override;
	ArtBearer bearerType() const override; //from CArtifactSet
	std::string nodeName() const override; //from CBonusSystemnode
	PlayerColor getOwner() const override;

	int32_t getInitiative(int turn = 0) const final;
	TerrainId getNativeTerrain() const final;
	TerrainId getCurrentTerrain() const;
};

VCMI_LIB_NAMESPACE_END
