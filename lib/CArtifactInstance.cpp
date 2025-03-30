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

#include "ArtifactUtils.h"
#include "CArtHandler.h"
#include "IGameCallback.h"
#include "networkPacks/ArtifactLocation.h"

VCMI_LIB_NAMESPACE_BEGIN

CCombinedArtifactInstance::PartInfo::PartInfo(IGameCallback * cb)
	:GameCallbackHolder(cb)
{}

CCombinedArtifactInstance::PartInfo::PartInfo(const CArtifactInstance * artifact, ArtifactPosition slot)
	: GameCallbackHolder(artifact->cb)
	, artifactID(artifact->getId())
	, slot(slot)
{
}

const CArtifactInstance * CCombinedArtifactInstance::PartInfo::getArtifact() const
{
	if (artifactID.hasValue())
		return cb->getArtInstance(artifactID);
	return nullptr;
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
		if(constituent.artifactID == supposedPart->getId())
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

		auto bonus = std::make_shared<Bonus>();
		bonus->type = BonusType::LEVEL_COUNTER;
		bonus->val = 1;
		bonus->duration = BonusDuration::COMMANDER_KILLED;
		artInst->accumulateBonus(bonus);

		for(const auto & bonus : artInst->getType()->getBonusesPerLevel())
		{
			// Every n levels
			if(artInst->valOfBonuses(BonusType::LEVEL_COUNTER) % bonus.first == 0)
			{
				artInst->accumulateBonus(std::make_shared<Bonus>(bonus.second));
			}
		}
		for(const auto & bonus : artInst->getType()->getThresholdBonuses())
		{
			// At n level
			if(artInst->valOfBonuses(BonusType::LEVEL_COUNTER) == bonus.first)
			{
				artInst->addNewBonus(std::make_shared<Bonus>(bonus.second));
			}
		}
	}
}

CArtifactInstance::CArtifactInstance(IGameCallback *cb, const CArtifact * art)
	:CArtifactInstance(cb)
{
	setType(art);
}

CArtifactInstance::CArtifactInstance(IGameCallback *cb)
	: CCombinedArtifactInstance(cb)
	, CBonusSystemNode(ARTIFACT_INSTANCE)
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

void CArtifactInstance::deserializationFix()
{
	setType(artTypeID.toArtifact());
	for(PartInfo & part : partsInfo)
		attachToSource(*part.getArtifact());
}

VCMI_LIB_NAMESPACE_END
