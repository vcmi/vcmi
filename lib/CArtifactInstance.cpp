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
#include "networkPacks/ArtifactLocation.h"

VCMI_LIB_NAMESPACE_BEGIN

void CCombinedArtifactInstance::addPart(CArtifactInstance * art, const ArtifactPosition & slot)
{
	auto artInst = static_cast<CArtifactInstance*>(this);
	assert(vstd::contains_if(artInst->artType->getConstituents(),
		[=](const CArtifact * partType)
		{
			return partType->getId() == art->getTypeId();
		}));
	assert(art->getParentNodes().size() == 1  &&  art->getParentNodes().front() == art->artType);
	partsInfo.emplace_back(art, slot);
	artInst->attachTo(*art);
}

bool CCombinedArtifactInstance::isPart(const CArtifactInstance * supposedPart) const
{
	if(supposedPart == this)
		return true;

	for(const PartInfo & constituent : partsInfo)
	{
		if(constituent.art == supposedPart)
			return true;
	}

	return false;
}

const std::vector<CCombinedArtifactInstance::PartInfo> & CCombinedArtifactInstance::getPartsInfo() const
{
	return partsInfo;
}

void CCombinedArtifactInstance::addPlacementMap(CArtifactSet::ArtPlacementMap & placementMap)
{
	if(!placementMap.empty())
		for(auto & part : partsInfo)
		{
			assert(placementMap.find(part.art) != placementMap.end());
			part.slot = placementMap.at(part.art);
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
	
	if(artInst->artType->isGrowing())
	{

		auto bonus = std::make_shared<Bonus>();
		bonus->type = BonusType::LEVEL_COUNTER;
		bonus->val = 1;
		bonus->duration = BonusDuration::COMMANDER_KILLED;
		artInst->accumulateBonus(bonus);

		for(const auto & bonus : artInst->artType->getBonusesPerLevel())
		{
			// Every n levels
			if(artInst->valOfBonuses(BonusType::LEVEL_COUNTER) % bonus.first == 0)
			{
				artInst->accumulateBonus(std::make_shared<Bonus>(bonus.second));
			}
		}
		for(const auto & bonus : artInst->artType->getThresholdBonuses())
		{
			// At n level
			if(artInst->valOfBonuses(BonusType::LEVEL_COUNTER) == bonus.first)
			{
				artInst->addNewBonus(std::make_shared<Bonus>(bonus.second));
			}
		}
	}
}

void CArtifactInstance::init()
{
	// Artifact to be randomized
	id = static_cast<ArtifactInstanceID>(ArtifactID::NONE);
	setNodeType(ARTIFACT_INSTANCE);
}

CArtifactInstance::CArtifactInstance(const CArtifact * art)
{
	init();
	setType(art);
}

CArtifactInstance::CArtifactInstance()
{
	init();
}

void CArtifactInstance::setType(const CArtifact * art)
{
	artType = art;
	attachToSource(*art);
}

std::string CArtifactInstance::nodeName() const
{
	return "Artifact instance of " + (artType ? artType->getJsonKey() : std::string("uninitialized")) + " type";
}

std::string CArtifactInstance::getDescription() const
{
	std::string text = artType->getDescriptionTranslated();
	if(artType->isScroll())
		ArtifactUtils::insertScrrollSpellName(text, getScrollSpellID());
	return text;
}

ArtifactID CArtifactInstance::getTypeId() const
{
	return artType->getId();
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
	return artType->canBePutAt(artSet, slot, assumeDestRemoved);
}

bool CArtifactInstance::isCombined() const
{
	return artType->isCombined();
}

bool CArtifactInstance::isScroll() const
{
	return artType->isScroll();
}

void CArtifactInstance::putAt(CArtifactSet & set, const ArtifactPosition slot)
{
	auto placementMap = set.putArtifact(slot, this);
	addPlacementMap(placementMap);
}

void CArtifactInstance::removeFrom(CArtifactSet & set, const ArtifactPosition slot)
{
	set.removeArtifact(slot);
	for(auto & part : partsInfo)
	{
		if(part.slot != ArtifactPosition::PRE_FIRST)
			part.slot = ArtifactPosition::PRE_FIRST;
	}
}

void CArtifactInstance::move(CArtifactSet & srcSet, const ArtifactPosition srcSlot, CArtifactSet & dstSet, const ArtifactPosition dstSlot)
{
	removeFrom(srcSet, srcSlot);
	putAt(dstSet, dstSlot);
}

void CArtifactInstance::deserializationFix()
{
	setType(artType);
	for(PartInfo & part : partsInfo)
		attachTo(*part.art);
}

VCMI_LIB_NAMESPACE_END
