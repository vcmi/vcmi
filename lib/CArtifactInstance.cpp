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
#include "NetPacksBase.h"

VCMI_LIB_NAMESPACE_BEGIN

void CCombinedArtifactInstance::addArtInstAsPart(CArtifactInstance * art, const ArtifactPosition & slot)
{
	auto artInst = static_cast<CArtifactInstance*>(this);
	assert(vstd::contains_if(*artInst->artType->constituents,
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

SpellID CScrollArtifactInstance::getScrollSpellID() const
{
	auto artInst = static_cast<const CArtifactInstance*>(this);
	const auto bonus = artInst->getBonusLocalFirst(Selector::type()(BonusType::SPELL));
	if(!bonus)
	{
		logMod->warn("Warning: %s doesn't bear any spell!", artInst->nodeName());
		return SpellID::NONE;
	}
	return SpellID(bonus->subtype);
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

		for(const auto & bonus : artInst->artType->bonusesPerLevel)
		{
			// Every n levels
			if(artInst->valOfBonuses(BonusType::LEVEL_COUNTER) % bonus.first == 0)
			{
				artInst->accumulateBonus(std::make_shared<Bonus>(bonus.second));
			}
		}
		for(const auto & bonus : artInst->artType->thresholdBonuses)
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

CArtifactInstance::CArtifactInstance(CArtifact * art)
{
	init();
	setType(art);
}

CArtifactInstance::CArtifactInstance()
{
	init();
}

void CArtifactInstance::setType(CArtifact * art)
{
	artType = art;
	attachTo(*art);
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

bool CArtifactInstance::canBePutAt(const ArtifactLocation & al, bool assumeDestRemoved) const
{
	return artType->canBePutAt(al.getHolderArtSet(), al.slot, assumeDestRemoved);
}

bool CArtifactInstance::isCombined() const
{
	return artType->isCombined();
}

void CArtifactInstance::putAt(const ArtifactLocation & al)
{
	al.getHolderArtSet()->putArtifact(al.slot, this);
}

void CArtifactInstance::removeFrom(const ArtifactLocation & al)
{
	al.getHolderArtSet()->removeArtifact(al.slot);
	for(auto & part : partsInfo)
	{
		if(part.slot != ArtifactPosition::PRE_FIRST)
			part.slot = ArtifactPosition::PRE_FIRST;
	}
}

void CArtifactInstance::move(const ArtifactLocation & src, const ArtifactLocation & dst)
{
	removeFrom(src);
	putAt(dst);
}

void CArtifactInstance::deserializationFix()
{
	setType(artType);
	for(PartInfo & part : partsInfo)
		attachTo(*part.art);
}

VCMI_LIB_NAMESPACE_END
