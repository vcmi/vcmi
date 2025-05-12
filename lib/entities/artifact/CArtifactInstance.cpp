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
	
	if(artInst->getType()->isGrowing())
	{
		auto growingBonus = std::make_shared<Bonus>();
		growingBonus->type = BonusType::ARTIFACT_GROWING;
		growingBonus->val = 1;
		growingBonus->duration = BonusDuration::PERMANENT;
		artInst->accumulateBonus(growingBonus);

		for(const auto & bonus : artInst->getType()->getBonusesPerLevel())
		{
			// Every n levels
			if(artInst->valOfBonuses(BonusType::ARTIFACT_GROWING) % bonus.first == 0)
			{
				artInst->accumulateBonus(std::make_shared<Bonus>(bonus.second));
			}
		}
		for(const auto & bonus : artInst->getType()->getThresholdBonuses())
		{
			// At n level
			if(artInst->valOfBonuses(BonusType::ARTIFACT_GROWING) == bonus.first)
			{
				artInst->addNewBonus(std::make_shared<Bonus>(bonus.second));
			}
		}
	}
}

void CChargedArtifactInstance::discharge(const uint16_t charges)
{
	auto artInst = static_cast<CArtifactInstance*>(this);

	if(const auto chargeBonuses = artInst->getAllBonuses(Selector::type()(BonusType::ARTIFACT_CHARGE), nullptr))
	{
		if(chargeBonuses->front()->val > charges)
			chargeBonuses->front()->val -= charges;
		else
			chargeBonuses->front()->val = 0;
	}
}

void CChargedArtifactInstance::addCharges(const uint16_t charges)
{
	auto artInst = static_cast<CArtifactInstance*>(this);

	if(artInst->getType()->isCharged())
	{
		auto bonus = std::make_shared<Bonus>();
		bonus->type = BonusType::ARTIFACT_CHARGE;
		bonus->val = charges;
		bonus->duration = BonusDuration::PERMANENT;
		artInst->accumulateBonus(bonus);
	}
}

uint16_t CChargedArtifactInstance::getCharges() const
{
	auto artInst = static_cast<const CArtifactInstance*>(this);

	return artInst->valOfBonuses(BonusType::ARTIFACT_CHARGE);
}

CArtifactInstance::CArtifactInstance(IGameInfoCallback *cb, const CArtifact * art)
	:CArtifactInstance(cb)
{
	setType(art);
	addCharges(getType()->getDefaultStartCharges());
}

CArtifactInstance::CArtifactInstance(IGameInfoCallback *cb)
	: CBonusSystemNode(ARTIFACT_INSTANCE)
	, CCombinedArtifactInstance(cb)
{
}

void CArtifactInstance::setType(const CArtifact * art)
{
	artTypeID = art->getId();
	attachToSource(*art);
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
