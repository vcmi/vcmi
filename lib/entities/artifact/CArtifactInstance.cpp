/*
 * CArtifactInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#include "StdInc.h"
#include "CArtifactInstance.h"

#include "CArtifact.h"
#include "CArtifactSet.h"

#include "../../callback/IGameInfoCallback.h"
#include "../../gameState/CGameState.h"

VCMI_LIB_NAMESPACE_BEGIN

CCombinedArtifactInstance::PartInfo::PartInfo(const CArtifactInstance * artifact, ArtifactPosition slot)
	: artifactPtr(artifact)
	, artifactID(artifact->getId())
	, slot(slot)
{
}

const CArtifactInstance * CCombinedArtifactInstance::PartInfo::getArtifact() const
{
	assert(artifactPtr != nullptr || !artifactID.hasValue());
	return artifactPtr;
}

ArtifactInstanceID CCombinedArtifactInstance::PartInfo::getArtifactID() const
{
	return artifactID;
}

void CCombinedArtifactInstance::addPart(const CArtifactInstance * art, const ArtifactPosition & slot)
{
	auto artInst = static_cast<CArtifactInstance*>(this);
	assert(vstd::contains_if(artInst->getType()->getConstituents(),
		[=](const CArtifact * partType)
		{
			return partType->getId() == art->getTypeId();
		}));
	assert(art->getParentNodes().size() == 1  &&  art->getParentNodes().front() == art->getType());
	partsInfo.emplace_back(art, slot);
	artInst->attachToSource(*art);
}

bool CCombinedArtifactInstance::isPart(const CArtifactInstance * supposedPart) const
{
	if(supposedPart == this)
		return true;

	for(const PartInfo & constituent : partsInfo)
	{
		if(constituent.getArtifactID() == supposedPart->getId())
			return true;
	}

	return false;
}

bool CCombinedArtifactInstance::hasParts() const
{
	return !partsInfo.empty();
}

const std::vector<CCombinedArtifactInstance::PartInfo> & CCombinedArtifactInstance::getPartsInfo() const
{
	return partsInfo;
}

void CCombinedArtifactInstance::addPlacementMap(const CArtifactSet::ArtPlacementMap & placementMap)
{
	if(!placementMap.empty())
		for(auto & part : partsInfo)
		{
			if(placementMap.find(part.getArtifact()) != placementMap.end())
				part.slot = placementMap.at(part.getArtifact());
		}
}

SpellID CScrollArtifactInstance::getScrollSpellID() const
{
	auto artInst = static_cast<const CArtifactInstance*>(this);
	const auto bonus = artInst->getFirstBonus(Selector::type()(BonusType::SPELL));
	if(!bonus)
		return SpellID::NONE;
	return bonus->subtype.as<SpellID>();
}

void CGrowingArtifactInstance::growingUp()
{
	auto artInst = static_cast<CArtifactInstance*>(this);
	const auto artType = artInst->getType();
	
	if(artType->isGrowing())
	{
		const auto growingBonus = artInst->getBonusesOfType(BonusType::ARTIFACT_GROWING);
		assert(!growingBonus->empty());
		growingBonus->front()->val++;

		for(const auto & bonus : artType->getBonusesPerLevel())
		{
			// Every n levels
			if(artInst->valOfBonuses(BonusType::ARTIFACT_GROWING) % bonus.first == 0)
			{
				artInst->accumulateBonus(std::make_shared<Bonus>(*bonus.second));
			}
		}
		for(const auto & bonus : artType->getThresholdBonuses())
		{
			// At n level
			if(artInst->valOfBonuses(BonusType::ARTIFACT_GROWING) == bonus.first)
			{
				artInst->addNewBonus(std::make_shared<Bonus>(*bonus.second));
			}
		}

		if(artType->isCharged())
			artInst->onChargesChanged();
	}
}

void CChargedArtifactInstance::onChargesChanged()
{
	auto artInst = static_cast<CArtifactInstance*>(this);
	const auto artType = artInst->getType();

	if(!artType->isCharged())
		return;

	const auto bonusSelector = artType->getDischargeCondition() == DischargeArtifactCondition::SPELLCAST ?
		Selector::type()(BonusType::SPELL) : Selector::all;

	auto instBonuses = artInst->getAllBonuses(bonusSelector, nullptr);

	if(artInst->getCharges() == 0)
	{
		for(const auto & bonus : *instBonuses)
			if(bonus->type != BonusType::ARTIFACT_GROWING && bonus->type != BonusType::ARTIFACT_CHARGE)
				artInst->removeBonus(bonus);
	}
	else
	{
		for(const auto & refBonus : *artType->getAllBonuses(bonusSelector, nullptr))
		{
			if(const auto bonusFound = std::find_if(instBonuses->begin(), instBonuses->end(),
				[refBonus](const auto & instBonus)
				{
					return refBonus->type == instBonus->type;
				}); bonusFound == instBonuses->end())
			{
				artInst->accumulateBonus(refBonus);
			}
		}
	}
}

void CChargedArtifactInstance::discharge(const uint16_t charges)
{
	auto artInst = static_cast<CArtifactInstance*>(this);

	if(const auto chargedBonus = artInst->getBonusesOfType(BonusType::ARTIFACT_CHARGE); !chargedBonus->empty())
	{
		if(chargedBonus->front()->val > charges)
			chargedBonus->front()->val -= charges;
		else
			chargedBonus->front()->val = 0;
		onChargesChanged();
	}
}

void CChargedArtifactInstance::addCharges(const uint16_t charges)
{
	auto artInst = static_cast<CArtifactInstance*>(this);

	if(artInst->getType()->isCharged())
	{
		const auto chargedBonus = artInst->getBonusesOfType(BonusType::ARTIFACT_CHARGE);
		assert(!chargedBonus->empty());
		chargedBonus->front()->val += charges;
		onChargesChanged();
	}
}

uint16_t CChargedArtifactInstance::getCharges() const
{
	auto artInst = static_cast<const CArtifactInstance*>(this);

	return artInst->valOfBonuses(BonusType::ARTIFACT_CHARGE);
}

void CArtifactInstance::init()
{
	const auto art = artTypeID.toArtifact();
	assert(art);

	if(art->isCharged())
	{
		// Charged artifacts contain all bonuses inside instance bonus node
		if(art->getDischargeCondition() == DischargeArtifactCondition::SPELLCAST)
		{
			for(const auto & bonus : *art->getAllBonuses(Selector::all, nullptr))
				if(bonus->type != BonusType::SPELL)
					accumulateBonus(bonus);
		}
	}
	else
	{
		attachToSource(*art);
	}
}

CArtifactInstance::CArtifactInstance(IGameInfoCallback *cb, const CArtifact * art)
	:CArtifactInstance(cb)
{
	artTypeID = art->getId();
	init();
}

CArtifactInstance::CArtifactInstance(IGameInfoCallback *cb)
	: CBonusSystemNode(ARTIFACT_INSTANCE)
	, CCombinedArtifactInstance(cb)
{
}

std::string CArtifactInstance::nodeName() const
{
	return "Artifact instance of " + (getType() ? getType()->getJsonKey() : std::string("uninitialized")) + " type";
}

ArtifactID CArtifactInstance::getTypeId() const
{
	return artTypeID;
}

const CArtifact * CArtifactInstance::getType() const
{
	return artTypeID.hasValue() ? artTypeID.toArtifact() : nullptr;
}

ArtifactInstanceID CArtifactInstance::getId() const
{
	return id;
}

void CArtifactInstance::setId(ArtifactInstanceID id)
{
	this->id = id;
}

bool CArtifactInstance::canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved) const
{
	return getType()->canBePutAt(artSet, slot, assumeDestRemoved);
}

bool CArtifactInstance::isCombined() const
{
	return getType()->isCombined();
}

bool CArtifactInstance::isScroll() const
{
	return getType()->isScroll();
}

void CArtifactInstance::attachToBonusSystem(CGameState & gs)
{
	for(PartInfo & part : partsInfo)
	{
		part = PartInfo(gs.getArtInstance(part.getArtifactID()), part.slot);
		attachToSource(*gs.getArtInstance(part.getArtifactID()));
	}
}

void CArtifactInstance::saveCompatibilityFixArtifactID(std::shared_ptr<CArtifactInstance> self)
{
	self->cb->gameState().saveCompatibilityLastAllocatedArtifactID = ArtifactInstanceID(self->cb->gameState().saveCompatibilityLastAllocatedArtifactID.getNum()+1);
	self->id = self->cb->gameState().saveCompatibilityLastAllocatedArtifactID;
	self->cb->gameState().saveCompatibilityUnregisteredArtifacts.push_back(self);
}

VCMI_LIB_NAMESPACE_END
