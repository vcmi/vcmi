/*
 * CArtifact.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArtifact.h"

#include "ArtifactUtils.h"
#include "CArtifactFittingSet.h"

#include "../../texts/CGeneralTextHandler.h"
#include "../../GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CCombinedArtifact::isCombined() const
{
	return !(constituents.empty());
}

const std::vector<const CArtifact*> & CCombinedArtifact::getConstituents() const
{
	return constituents;
}

const std::set<const CArtifact*> & CCombinedArtifact::getPartOf() const
{
	return partOf;
}

void CCombinedArtifact::setFused(bool isFused)
{
	fused = isFused;
}

bool CCombinedArtifact::isFused() const
{
	return fused;
}

bool CCombinedArtifact::hasParts() const
{
	return isCombined() && !isFused();
}

bool CScrollArtifact::isScroll() const
{
	return static_cast<const CArtifact*>(this)->getId() == ArtifactID::SPELL_SCROLL;
}

bool CGrowingArtifact::isGrowing() const
{
	return !bonusesPerLevel.empty() || !thresholdBonuses.empty();
}

std::vector <std::pair<ui16, Bonus>> & CGrowingArtifact::getBonusesPerLevel()
{
	return bonusesPerLevel;
}

const std::vector <std::pair<ui16, Bonus>> & CGrowingArtifact::getBonusesPerLevel() const
{
	return bonusesPerLevel;
}

std::vector <std::pair<ui16, Bonus>> & CGrowingArtifact::getThresholdBonuses()
{
	return thresholdBonuses;
}

const std::vector <std::pair<ui16, Bonus>> & CGrowingArtifact::getThresholdBonuses() const
{
	return thresholdBonuses;
}

int32_t CArtifact::getIndex() const
{
	return id.toEnum();
}

int32_t CArtifact::getIconIndex() const
{
	return iconIndex;
}

std::string CArtifact::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CArtifact::getModScope() const
{
	return modScope;
}

void CArtifact::registerIcons(const IconRegistar & cb) const
{
	cb(getIconIndex(), 0, "ARTIFACT", image);
	cb(getIconIndex(), 0, "ARTIFACTLARGE", large);
}

ArtifactID CArtifact::getId() const
{
	return id;
}

const IBonusBearer * CArtifact::getBonusBearer() const
{
	return this;
}

std::string CArtifact::getDescriptionTranslated() const
{
	return LIBRARY->generaltexth->translate(getDescriptionTextID());
}

std::string CArtifact::getEventTranslated() const
{
	return LIBRARY->generaltexth->translate(getEventTextID());
}

std::string CArtifact::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CArtifact::getDescriptionTextID() const
{
	return TextIdentifier("artifact", modScope, identifier, "description").get();
}

std::string CArtifact::getEventTextID() const
{
	return TextIdentifier("artifact", modScope, identifier, "event").get();
}

std::string CArtifact::getNameTextID() const
{
	return TextIdentifier("artifact", modScope, identifier, "name").get();
}

uint32_t CArtifact::getPrice() const
{
	return price;
}

CreatureID CArtifact::getWarMachine() const
{
	return warMachine;
}

bool CArtifact::isBig() const
{
	return warMachine != CreatureID::NONE;
}

bool CArtifact::isTradable() const
{
	switch(id.toEnum())
	{
		case ArtifactID::SPELLBOOK:
			return false;
		default:
			return !isBig();
	}
}

bool CArtifact::canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved) const
{
	auto simpleArtCanBePutAt = [this](const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved) -> bool
	{
		if(artSet->bearerType() == ArtBearer::HERO && ArtifactUtils::isSlotBackpack(slot))
		{
			if(isBig() || (!assumeDestRemoved && !ArtifactUtils::isBackpackFreeSlots(artSet)))
				return false;
			return true;
		}

		if(!vstd::contains(possibleSlots.at(artSet->bearerType()), slot))
			return false;

		return artSet->isPositionFree(slot, assumeDestRemoved);
	};

	auto artCanBePutAt = [this, simpleArtCanBePutAt](const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved) -> bool
	{
		if(hasParts())
		{
			if(!simpleArtCanBePutAt(artSet, slot, assumeDestRemoved))
				return false;
			if(ArtifactUtils::isSlotBackpack(slot))
				return true;

			CArtifactFittingSet fittingSet(artSet->getCallback(), artSet->bearerType());
			fittingSet.artifactsWorn = artSet->artifactsWorn;
			if(assumeDestRemoved)
				fittingSet.removeArtifact(slot);

			for(const auto art : constituents)
			{
				auto possibleSlot = ArtifactUtils::getArtAnyPosition(&fittingSet, art->getId());
				if(ArtifactUtils::isSlotEquipment(possibleSlot))
				{
					if (fittingSet.getSlot(possibleSlot) == nullptr)
						fittingSet.artifactsWorn.insert(std::make_pair(possibleSlot, ArtSlotInfo(fittingSet.cb)));
					fittingSet.lockSlot(possibleSlot);
				}
				else
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			return simpleArtCanBePutAt(artSet, slot, assumeDestRemoved);
		}
	};

	if(slot == ArtifactPosition::TRANSITION_POS)
		return true;

	if(slot == ArtifactPosition::FIRST_AVAILABLE)
	{
		for(const auto & possibleSlot : possibleSlots.at(artSet->bearerType()))
		{
			if(artCanBePutAt(artSet, possibleSlot, assumeDestRemoved))
				return true;
		}
		return artCanBePutAt(artSet, ArtifactPosition::BACKPACK_START, assumeDestRemoved);
	}
	else if(ArtifactUtils::isSlotBackpack(slot))
	{
		return artCanBePutAt(artSet, ArtifactPosition::BACKPACK_START, assumeDestRemoved);
	}
	else
	{
		return artCanBePutAt(artSet, slot, assumeDestRemoved);
	}
}

CArtifact::CArtifact()
	: iconIndex(ArtifactID::NONE),
	price(0)
{
	setNodeType(ARTIFACT);
	possibleSlots[ArtBearer::HERO]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::CREATURE]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::COMMANDER];
	possibleSlots[ArtBearer::ALTAR];
}

//This destructor should be placed here to avoid side effects
CArtifact::~CArtifact() = default;

int CArtifact::getArtClassSerial() const
{
	if(id == ArtifactID::SPELL_SCROLL)
		return 4;
	switch(aClass)
	{
		case EArtifactClass::ART_TREASURE:
			return 0;
		case EArtifactClass::ART_MINOR:
			return 1;
		case EArtifactClass::ART_MAJOR:
			return 2;
		case EArtifactClass::ART_RELIC:
			return 3;
		case EArtifactClass::ART_SPECIAL:
			return 5;
	}

	return -1;
}

std::string CArtifact::nodeName() const
{
	return "Artifact: " + getNameTranslated();
}

void CArtifact::addNewBonus(const std::shared_ptr<Bonus>& b)
{
	b->source = BonusSource::ARTIFACT;
	b->duration = BonusDuration::PERMANENT;
	b->description.appendTextID(getNameTextID());
	b->description.appendRawString(" %+d");
	CBonusSystemNode::addNewBonus(b);
}

const std::map<ArtBearer, std::vector<ArtifactPosition>> & CArtifact::getPossibleSlots() const
{
	return possibleSlots;
}

void CArtifact::updateFrom(const JsonNode& data)
{
	//TODO:CArtifact::updateFrom
}

void CArtifact::setImage(int32_t newIconIndex, const std::string & newImage, const std::string & newLargeImage)
{
	iconIndex = newIconIndex;
	image = newImage;
	large = newLargeImage;
}


VCMI_LIB_NAMESPACE_END
