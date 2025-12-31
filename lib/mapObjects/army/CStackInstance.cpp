/*
 * CStackInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CStackInstance.h"

#include "CArmedInstance.h"

#include "../../CConfigHandler.h"
#include "../../GameLibrary.h"
#include "../../IGameSettings.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../entities/faction/CFaction.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../../IBonusTypeHandler.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

CStackInstance::CStackInstance(IGameInfoCallback * cb)
	: CStackInstance(cb, BonusNodeType::STACK_INSTANCE, false)
{
}

CStackInstance::CStackInstance(IGameInfoCallback * cb, BonusNodeType nodeType, bool isHypothetic)
	: CBonusSystemNode(nodeType, isHypothetic)
	, CStackBasicDescriptor(nullptr, 0)
	, CArtifactSet(cb)
	, GameCallbackHolder(cb)
	, nativeTerrain(this, Selector::type()(BonusType::TERRAIN_NATIVE))
	, initiative(this, Selector::type()(BonusType::STACKS_SPEED))
	, totalExperience(0)
{
}

CStackInstance::CStackInstance(IGameInfoCallback * cb, const CreatureID & id, TQuantity Count, bool isHypothetic)
	: CStackInstance(cb, BonusNodeType::STACK_INSTANCE, false)
{
	setType(id);
	setCount(Count);
}

CCreature::CreatureQuantityId CStackInstance::getQuantityID() const
{
	return CCreature::getQuantityID(getCount());
}

int CStackInstance::getExpRank() const
{
	if(!LIBRARY->engineSettings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
		return 0;
	int tier = getType()->getLevel();
	if(vstd::iswithin(tier, 1, 7))
	{
		for(int i = static_cast<int>(LIBRARY->creh->expRanks[tier].size()) - 2; i > -1; --i) //sic!
		{ //exp values vary from 1st level to max exp at 11th level
			if(getAverageExperience() >= LIBRARY->creh->expRanks[tier][i])
				return ++i; //faster, but confusing - 0 index mean 1st level of experience
		}
		return 0;
	}
	else //higher tier
	{
		for(int i = static_cast<int>(LIBRARY->creh->expRanks[0].size()) - 2; i > -1; --i)
		{
			if(getAverageExperience() >= LIBRARY->creh->expRanks[0][i])
				return ++i;
		}
		return 0;
	}
}

int CStackInstance::getLevel() const
{
	return std::max(1, getType()->getLevel());
}

void CStackInstance::giveAverageStackExperience(TExpType desiredAmountPerUnit)
{
	if(!canGainExperience())
		return;

	int level = std::clamp(getLevel(), 1, 7);
	TExpType maxAmountPerUnit = LIBRARY->creh->expRanks[level].back();
	TExpType actualAmountPerUnit = std::min(desiredAmountPerUnit, maxAmountPerUnit * LIBRARY->creh->maxExpPerBattle[level] / 100);
	TExpType maxExperience = maxAmountPerUnit * getCount();
	TExpType maxExperienceToGain = maxExperience - totalExperience;
	TExpType actualGainedExperience = std::min(maxExperienceToGain, actualAmountPerUnit * getCount());

	totalExperience += actualGainedExperience;
}

void CStackInstance::giveTotalStackExperience(TExpType experienceToGive)
{
	if(!canGainExperience())
		return;

	totalExperience += experienceToGive;
}

TExpType CStackInstance::getTotalExperience() const
{
	return totalExperience;
}

TExpType CStackInstance::getAverageExperience() const
{
	return totalExperience / getCount();
}

bool CStackInstance::canGainExperience() const
{
	return cb->getSettings().getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE);
}

void CStackInstance::setType(const CreatureID & creID)
{
	if(creID == CreatureID::NONE)
		setType(nullptr); //FIXME: unused branch?
	else
		setType(creID.toCreature());
}

void CStackInstance::setType(const CCreature * c)
{
	if(getCreature())
	{
		detachFromSource(*getCreature());
		if(LIBRARY->engineSettings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
			totalExperience = totalExperience * LIBRARY->creh->expAfterUpgrade / 100;
	}

	CStackBasicDescriptor::setType(c);

	if(getCreature())
		attachToSource(*getCreature());
}

void CStackInstance::setCount(TQuantity newCount)
{
	assert(newCount >= 0);

	if(newCount < getCount())
	{
		TExpType averageExperience = totalExperience / getCount();
		totalExperience = averageExperience * newCount;
	}

	CStackBasicDescriptor::setCount(newCount);
	nodeHasChanged();
}

std::string CStackInstance::bonusToString(const std::shared_ptr<Bonus> & bonus) const
{
	if(!bonus->description.empty())
		return bonus->description.toString();
	else
		return LIBRARY->getBth()->bonusToString(bonus, this);
}

ImagePath CStackInstance::bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const
{
	if(!bonus->customIconPath.empty())
		return bonus->customIconPath;
	return LIBRARY->getBth()->bonusToGraphics(bonus);
}

CArmedInstance * CStackInstance::getArmy()
{
	return armyInstance;
}

const CArmedInstance * CStackInstance::getArmy() const
{
	return armyInstance;
}

void CStackInstance::setArmy(CArmedInstance * ArmyObj)
{
	auto oldArmy = getArmy();

	if(oldArmy)
	{
		detachFrom(*oldArmy);
		armyInstance = nullptr;
	}

	if(ArmyObj)
	{
		attachTo(const_cast<CArmedInstance &>(*ArmyObj));
		armyInstance = ArmyObj;
	}
}

std::string CStackInstance::getQuantityTXT(bool capitalized) const
{
	CCreature::CreatureQuantityId quantity = getQuantityID();

	if(static_cast<int>(quantity))
	{
		if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
			return CCreature::getQuantityRangeStringForId(quantity);

		return LIBRARY->generaltexth->arraytxt[174 + static_cast<int>(quantity) * 3 - 1 - capitalized];
	}
	else
		return "";
}

bool CStackInstance::valid(bool allowUnrandomized) const
{
	if(!randomStack)
	{
		return (getType() && getType() == getId().toEntity(LIBRARY));
	}
	else
		return allowUnrandomized;
}

std::string CStackInstance::nodeName() const
{
	std::ostringstream oss;
	oss << "Stack of " << getCount() << " of ";
	if(getType())
		oss << getType()->getNamePluralTextID();
	else
		oss << "[UNDEFINED TYPE]";

	return oss.str();
}

PlayerColor CStackInstance::getOwner() const
{
	auto army = getArmy();
	return army ? army->getOwner() : PlayerColor::NEUTRAL;
}

int32_t CStackInstance::getInitiative(int turn) const
{
	if(turn == 0)
		return initiative.getValue();

	return ACreature::getInitiative(turn);
}

TerrainId CStackInstance::getNativeTerrain() const
{
	if(nativeTerrain.hasBonus())
		return TerrainId::ANY_TERRAIN;

	return getFactionID().toEntity(LIBRARY)->getNativeTerrain();
}

TerrainId CStackInstance::getCurrentTerrain() const
{
	if (armyInstance)
		return armyInstance->getCurrentTerrain();
	else
		return TerrainId::NONE;		//for example a stackInstance created in order to display unit's statistics on a town screen
}

CreatureID CStackInstance::getCreatureID() const
{
	if(getType())
		return getType()->getId();
	else
		return CreatureID::NONE;
}

std::string CStackInstance::getName() const
{
	return (getCount() > 1) ? getType()->getNamePluralTranslated() : getType()->getNameSingularTranslated();
}

ui64 CStackInstance::getPower() const
{
	assert(getType());
	return static_cast<ui64>(getType()->getAIValue()) * getCount();
}

ui64 CStackInstance::getMarketValue() const
{
	assert(getType());
	return getType()->getFullRecruitCost().marketValue() * getCount();
}

ArtBearer CStackInstance::bearerType() const
{
	return ArtBearer::CREATURE;
}

CStackInstance::ArtPlacementMap CStackInstance::putArtifact(const ArtifactPosition & pos, const CArtifactInstance * art)
{
	assert(!getArt(pos));
	assert(art->canBePutAt(this, pos));

	attachToSource(*art);
	return CArtifactSet::putArtifact(pos, art);
}

void CStackInstance::removeArtifact(const ArtifactPosition & pos)
{
	assert(getArt(pos));

	detachFromSource(*getArt(pos));
	CArtifactSet::removeArtifact(pos);
}

void CStackInstance::serializeJson(JsonSerializeFormat & handler)
{
	//todo: artifacts
	CStackBasicDescriptor::serializeJson(handler); //must be first

	if(handler.saving)
	{
		if(randomStack)
		{
			int level = randomStack->level;
			int upgrade = randomStack->upgrade;

			handler.serializeInt("level", level, 0);
			handler.serializeInt("upgraded", upgrade, 0);
		}
	}
	else
	{
		//type set by CStackBasicDescriptor::serializeJson
		if(getType() == nullptr)
		{
			uint8_t level = 0;
			uint8_t upgrade = 0;

			handler.serializeInt("level", level, 0);
			handler.serializeInt("upgrade", upgrade, 0);

			randomStack = RandomStackInfo{level, upgrade};
		}
	}
}

FactionID CStackInstance::getFactionID() const
{
	if(getType())
		return getType()->getFactionID();

	return FactionID::NEUTRAL;
}

const IBonusBearer * CStackInstance::getBonusBearer() const
{
	return this;
}

VCMI_LIB_NAMESPACE_END
