/*
 * CArtHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArtHandler.h"

#include "filesystem/Filesystem.h"
#include "CGeneralTextHandler.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"
#include "GameSettings.h"
#include "CCreatureHandler.h"
#include "spells/CSpellHandler.h"
#include "mapObjects/MapObjects.h"
#include "NetPacksBase.h"
#include "StringConstants.h"
#include "CRandomGenerator.h"

#include "mapObjects/CObjectClassesHandler.h"
#include "mapping/CMap.h"
#include "serializer/JsonSerializeFormat.h"

// Note: list must match entries in ArtTraits.txt
#define ART_POS_LIST    \
	ART_POS(SPELLBOOK)  \
	ART_POS(MACH4)      \
	ART_POS(MACH3)      \
	ART_POS(MACH2)      \
	ART_POS(MACH1)      \
	ART_POS(MISC5)      \
	ART_POS(MISC4)      \
	ART_POS(MISC3)      \
	ART_POS(MISC2)      \
	ART_POS(MISC1)      \
	ART_POS(FEET)       \
	ART_POS(LEFT_RING)  \
	ART_POS(RIGHT_RING) \
	ART_POS(TORSO)      \
	ART_POS(LEFT_HAND)  \
	ART_POS(RIGHT_HAND) \
	ART_POS(NECK)       \
	ART_POS(SHOULDERS)  \
	ART_POS(HEAD)

VCMI_LIB_NAMESPACE_BEGIN

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
	return VLC->generaltexth->translate(getDescriptionTextID());
}

std::string CArtifact::getEventTranslated() const
{
	return VLC->generaltexth->translate(getEventTextID());
}

std::string CArtifact::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
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
	switch(id)
	{
	case ArtifactID::SPELLBOOK:
	case ArtifactID::GRAIL:
		return false;
	default:
		return !isBig();
	}
}

bool CArtifact::canBeDisassembled() const
{
	return !(constituents == nullptr);
}

bool CArtifact::canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved) const
{
	auto simpleArtCanBePutAt = [this](const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved) -> bool
	{
		if(ArtifactUtils::isSlotBackpack(slot))
		{
			if(isBig() || !ArtifactUtils::isBackpackFreeSlots(artSet))
				return false;
			return true;
		}

		if(!vstd::contains(possibleSlots.at(artSet->bearerType()), slot))
			return false;

		return artSet->isPositionFree(slot, assumeDestRemoved);
	};

	auto artCanBePutAt = [this, simpleArtCanBePutAt](const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved) -> bool
	{
		if(canBeDisassembled())
		{
			if(!simpleArtCanBePutAt(artSet, slot, assumeDestRemoved))
				return false;
			if(ArtifactUtils::isSlotBackpack(slot))
				return true;

			CArtifactFittingSet fittingSet(artSet->bearerType());
			fittingSet.artifactsWorn = artSet->artifactsWorn;
			if(assumeDestRemoved)
				fittingSet.removeArtifact(slot);
			assert(constituents);
			for(const auto art : *constituents)
			{
				auto possibleSlot = ArtifactUtils::getArtAnyPosition(&fittingSet, art->getId());
				if(ArtifactUtils::isSlotEquipment(possibleSlot))
				{
					fittingSet.setNewArtSlot(possibleSlot, nullptr, true);
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
		for(const auto & slot : possibleSlots.at(artSet->bearerType()))
		{
			if(artCanBePutAt(artSet, slot, assumeDestRemoved))
				return true;
		}
		return artCanBePutAt(artSet, GameConstants::BACKPACK_START, assumeDestRemoved);
	}
	else if(ArtifactUtils::isSlotBackpack(slot))
	{
		return artCanBePutAt(artSet, GameConstants::BACKPACK_START, assumeDestRemoved);
	}
	else
	{
		return artCanBePutAt(artSet, slot, assumeDestRemoved);
	}
}

CArtifact::CArtifact()
{
	setNodeType(ARTIFACT);
	possibleSlots[ArtBearer::HERO]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::CREATURE]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::COMMANDER];
}

//This destructor should be placed here to avoid side effects
CArtifact::~CArtifact() = default;

int CArtifact::getArtClassSerial() const
{
	if(id == ArtifactID::SPELL_SCROLL)
		return 4;
	switch(aClass)
	{
	case ART_TREASURE:
		return 0;
	case ART_MINOR:
		return 1;
	case ART_MAJOR:
		return 2;
	case ART_RELIC:
		return 3;
	case ART_SPECIAL:
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
	b->source = Bonus::ARTIFACT;
	b->duration = Bonus::PERMANENT;
	b->description = getNameTranslated();
	CBonusSystemNode::addNewBonus(b);
}

void CArtifact::updateFrom(const JsonNode& data)
{
	//TODO:CArtifact::updateFrom
}

void CArtifact::serializeJson(JsonSerializeFormat & handler)
{

}

void CGrowingArtifact::levelUpArtifact (CArtifactInstance * art)
{
	auto b = std::make_shared<Bonus>();
	b->type = Bonus::LEVEL_COUNTER;
	b->val = 1;
	b->duration = Bonus::COMMANDER_KILLED;
	art->accumulateBonus(b);

	for(const auto & bonus : bonusesPerLevel)
	{
		if (art->valOfBonuses(Bonus::LEVEL_COUNTER) % bonus.first == 0) //every n levels
		{
			art->accumulateBonus(std::make_shared<Bonus>(bonus.second));
		}
	}
	for(const auto & bonus : thresholdBonuses)
	{
		if (art->valOfBonuses(Bonus::LEVEL_COUNTER) == bonus.first) //every n levels
		{
			art->addNewBonus(std::make_shared<Bonus>(bonus.second));
		}
	}
}

CArtHandler::~CArtHandler() = default;

std::vector<JsonNode> CArtHandler::loadLegacyData()
{
	size_t dataSize = VLC->settings()->getInteger(EGameSettings::TEXTS_ARTIFACT);

	objects.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	#define ART_POS(x) #x ,
	const std::vector<std::string> artSlots = { ART_POS_LIST };
	#undef ART_POS

	static std::map<char, std::string> classes =
		{{'S',"SPECIAL"}, {'T',"TREASURE"},{'N',"MINOR"},{'J',"MAJOR"},{'R',"RELIC"},};

	CLegacyConfigParser parser("DATA/ARTRAITS.TXT");
	CLegacyConfigParser events("DATA/ARTEVENT.TXT");

	parser.endLine(); // header
	parser.endLine();

	for (size_t i = 0; i < dataSize; i++)
	{
		JsonNode artData;

		artData["text"]["name"].String() = parser.readString();
		artData["text"]["event"].String() = events.readString();
		artData["value"].Float() = parser.readNumber();

		for(const auto & artSlot : artSlots)
		{
			if(parser.readString() == "x")
			{
				artData["slot"].Vector().push_back(JsonNode());
				artData["slot"].Vector().back().String() = artSlot;
			}
		}
		artData["class"].String() = classes[parser.readString()[0]];
		artData["text"]["description"].String() = parser.readString();

		parser.endLine();
		events.endLine();
		h3Data.push_back(artData);
	}
	return h3Data;
}

void CArtHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto * object = loadFromJson(scope, data, name, objects.size());

	object->iconIndex = object->getIndex() + 5;

	objects.emplace_back(object);

	registerObject(scope, "artifact", name, object->id);
}

void CArtHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto * object = loadFromJson(scope, data, name, index);

	object->iconIndex = object->getIndex();

	assert(objects[index] == nullptr); // ensure that this id was not loaded before
	objects[index] = object;

	registerObject(scope, "artifact", name, object->id);
}

const std::vector<std::string> & CArtHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "artifact" };
	return typeNames;
}

CArtifact * CArtHandler::loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	CArtifact * art = nullptr;

	if(!VLC->settings()->getBoolean(EGameSettings::MODULE_COMMANDERS) || node["growing"].isNull())
	{
		art = new CArtifact();
	}
	else
	{
		auto * growing = new CGrowingArtifact();
		loadGrowingArt(growing, node);
		art = growing;
	}
	art->id = ArtifactID(index);
	art->identifier = identifier;
	art->modScope = scope;

	const JsonNode & text = node["text"];

	VLC->generaltexth->registerString(scope, art->getNameTextID(), text["name"].String());
	VLC->generaltexth->registerString(scope, art->getDescriptionTextID(), text["description"].String());
	VLC->generaltexth->registerString(scope, art->getEventTextID(), text["event"].String());

	const JsonNode & graphics = node["graphics"];
	art->image = graphics["image"].String();

	if(!graphics["large"].isNull())
		art->large = graphics["large"].String();
	else
		art->large = art->image;

	art->advMapDef = graphics["map"].String();

	art->price = static_cast<ui32>(node["value"].Float());

	loadSlots(art, node);
	loadClass(art, node);
	loadType(art, node);
	loadComponents(art, node);

	for(const auto & b : node["bonuses"].Vector())
	{
		auto bonus = JsonUtils::parseBonus(b);
		art->addNewBonus(bonus);
	}

	const JsonNode & warMachine = node["warMachine"];
	if(warMachine.getType() == JsonNode::JsonType::DATA_STRING && !warMachine.String().empty())
	{
		VLC->modh->identifiers.requestIdentifier("creature", warMachine, [=](si32 id)
		{
			art->warMachine = CreatureID(id);

			//this assumes that creature object is stored before registration
			VLC->creh->objects.at(id)->warMachine = art->id;
		});
	}

	VLC->modh->identifiers.requestIdentifier(scope, "object", "artifact", [=](si32 index)
	{
		JsonNode conf;
		conf.setMeta(scope);

		VLC->objtypeh->loadSubObject(art->identifier, conf, Obj::ARTIFACT, art->getIndex());

		if(!art->advMapDef.empty())
		{
			JsonNode templ;
			templ["animation"].String() = art->advMapDef;
			templ.setMeta(scope);

			// add new template.
			// Necessary for objects added via mods that don't have any templates in H3
			VLC->objtypeh->getHandlerFor(Obj::ARTIFACT, art->getIndex())->addTemplate(templ);
		}
		// object does not have any templates - this is not usable object (e.g. pseudo-art like lock)
		if(VLC->objtypeh->getHandlerFor(Obj::ARTIFACT, art->getIndex())->getTemplates().empty())
			VLC->objtypeh->removeSubObject(Obj::ARTIFACT, art->getIndex());
	});

	return art;
}

ArtifactPosition::ArtifactPosition(std::string slotName):
	num(ArtifactPosition::PRE_FIRST)
{
#define ART_POS(x) { #x, ArtifactPosition::x },
	static const std::map<std::string, ArtifactPosition> artifactPositionMap = { ART_POS_LIST };
#undef ART_POS
	auto it = artifactPositionMap.find (slotName);
	if (it != artifactPositionMap.end())
		num = it->second;
	else
		logMod->warn("Warning! Artifact slot %s not recognized!", slotName);
}

void CArtHandler::addSlot(CArtifact * art, const std::string & slotID) const
{
	static const std::vector<ArtifactPosition> miscSlots =
	{
		ArtifactPosition::MISC1, ArtifactPosition::MISC2, ArtifactPosition::MISC3, ArtifactPosition::MISC4, ArtifactPosition::MISC5
	};

	static const std::vector<ArtifactPosition> ringSlots =
	{
		ArtifactPosition::RIGHT_RING, ArtifactPosition::LEFT_RING
	};

	if (slotID == "MISC")
	{
		vstd::concatenate(art->possibleSlots[ArtBearer::HERO], miscSlots);
	}
	else if (slotID == "RING")
	{
		vstd::concatenate(art->possibleSlots[ArtBearer::HERO], ringSlots);
	}
	else
	{
		auto slot = ArtifactPosition(slotID);
		if (slot != ArtifactPosition::PRE_FIRST)
			art->possibleSlots[ArtBearer::HERO].push_back(slot);
	}
}

void CArtHandler::loadSlots(CArtifact * art, const JsonNode & node) const
{
	if (!node["slot"].isNull()) //we assume non-hero slots are irrelevant?
	{
		if (node["slot"].getType() == JsonNode::JsonType::DATA_STRING)
			addSlot(art, node["slot"].String());
		else
		{
			for (const JsonNode & slot : node["slot"].Vector())
				addSlot(art, slot.String());
		}
		std::sort(art->possibleSlots.at(ArtBearer::HERO).begin(), art->possibleSlots.at(ArtBearer::HERO).end());
	}
}

CArtifact::EartClass CArtHandler::stringToClass(const std::string & className)
{
	static const std::map<std::string, CArtifact::EartClass> artifactClassMap =
	{
		{"TREASURE", CArtifact::ART_TREASURE},
		{"MINOR", CArtifact::ART_MINOR},
		{"MAJOR", CArtifact::ART_MAJOR},
		{"RELIC", CArtifact::ART_RELIC},
		{"SPECIAL", CArtifact::ART_SPECIAL}
	};

	auto it = artifactClassMap.find (className);
	if (it != artifactClassMap.end())
		return it->second;

	logMod->warn("Warning! Artifact rarity %s not recognized!", className);
	return CArtifact::ART_SPECIAL;
}

void CArtHandler::loadClass(CArtifact * art, const JsonNode & node) const
{
	art->aClass = stringToClass(node["class"].String());
}

void CArtHandler::loadType(CArtifact * art, const JsonNode & node) const
{
#define ART_BEARER(x) { #x, ArtBearer::x },
	static const std::map<std::string, int> artifactBearerMap = { ART_BEARER_LIST };
#undef ART_BEARER

	for (const JsonNode & b : node["type"].Vector())
	{
		auto it = artifactBearerMap.find (b.String());
		if (it != artifactBearerMap.end())
		{
			int bearerType = it->second;
			switch (bearerType)
			{
				case ArtBearer::HERO://TODO: allow arts having several possible bearers
					break;
				case ArtBearer::COMMANDER:
					makeItCommanderArt (art); //original artifacts should have only one bearer type
					break;
				case ArtBearer::CREATURE:
					makeItCreatureArt (art);
					break;
			}
		}
		else
			logMod->warn("Warning! Artifact type %s not recognized!", b.String());
	}
}

void CArtHandler::loadComponents(CArtifact * art, const JsonNode & node)
{
	if (!node["components"].isNull())
	{
		art->constituents = std::make_unique<std::vector<CArtifact *>>();
		for(const auto & component : node["components"].Vector())
		{
			VLC->modh->identifiers.requestIdentifier("artifact", component, [=](si32 id)
			{
				// when this code is called both combinational art as well as component are loaded
				// so it is safe to access any of them
				art->constituents->push_back(objects[id]);
				objects[id]->constituentOf.push_back(art);
			});
		}
	}
}

void CArtHandler::loadGrowingArt(CGrowingArtifact * art, const JsonNode & node) const
{
	for (auto b : node["growing"]["bonusesPerLevel"].Vector())
	{
		art->bonusesPerLevel.emplace_back(static_cast<ui16>(b["level"].Float()), Bonus());
		JsonUtils::parseBonus(b["bonus"], &art->bonusesPerLevel.back().second);
	}
	for (auto b : node["growing"]["thresholdBonuses"].Vector())
	{
		art->thresholdBonuses.emplace_back(static_cast<ui16>(b["level"].Float()), Bonus());
		JsonUtils::parseBonus(b["bonus"], &art->thresholdBonuses.back().second);
	}
}

ArtifactID CArtHandler::pickRandomArtifact(CRandomGenerator & rand, int flags, std::function<bool(ArtifactID)> accepts)
{
	auto getAllowedArts = [&](std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, CArtifact::EartClass flag)
	{
		if (arts->empty()) //restock available arts
			fillList(*arts, flag);

		for (auto & arts_i : *arts)
		{
			if (accepts(arts_i->id))
			{
				CArtifact *art = arts_i;
				out.emplace_back(art);
			}
		}
	};

	auto getAllowed = [&](std::vector<ConstTransitivePtr<CArtifact> > &out)
	{
		if (flags & CArtifact::ART_TREASURE)
			getAllowedArts (out, &treasures, CArtifact::ART_TREASURE);
		if (flags & CArtifact::ART_MINOR)
			getAllowedArts (out, &minors, CArtifact::ART_MINOR);
		if (flags & CArtifact::ART_MAJOR)
			getAllowedArts (out, &majors, CArtifact::ART_MAJOR);
		if (flags & CArtifact::ART_RELIC)
			getAllowedArts (out, &relics, CArtifact::ART_RELIC);
		if(out.empty()) //no artifact of specified rarity, we need to take another one
		{
			getAllowedArts (out, &treasures, CArtifact::ART_TREASURE);
			getAllowedArts (out, &minors, CArtifact::ART_MINOR);
			getAllowedArts (out, &majors, CArtifact::ART_MAJOR);
			getAllowedArts (out, &relics, CArtifact::ART_RELIC);
		}
		if(out.empty()) //no arts are available at all
		{
			out.resize (64);
			std::fill_n (out.begin(), 64, objects[2]); //Give Grail - this can't be banned (hopefully)
		}
	};

	std::vector<ConstTransitivePtr<CArtifact> > out;
	getAllowed(out);
	ArtifactID artID = (*RandomGeneratorUtil::nextItem(out, rand))->id;
	erasePickedArt(artID);
	return artID;
}

ArtifactID CArtHandler::pickRandomArtifact(CRandomGenerator & rand, std::function<bool(ArtifactID)> accepts)
{
	return pickRandomArtifact(rand, 0xff, std::move(accepts));
}

ArtifactID CArtHandler::pickRandomArtifact(CRandomGenerator & rand, int flags)
{
	return pickRandomArtifact(rand, flags, [](const ArtifactID &) { return true; });
}

void CArtHandler::makeItCreatureArt(CArtifact * a, bool onlyCreature)
{
	if (onlyCreature)
	{
		a->possibleSlots[ArtBearer::HERO].clear();
		a->possibleSlots[ArtBearer::COMMANDER].clear();
	}
	a->possibleSlots[ArtBearer::CREATURE].push_back(ArtifactPosition::CREATURE_SLOT);
}

void CArtHandler::makeItCommanderArt(CArtifact * a, bool onlyCommander)
{
	if (onlyCommander)
	{
		a->possibleSlots[ArtBearer::HERO].clear();
		a->possibleSlots[ArtBearer::CREATURE].clear();
	}
	for (int i = ArtifactPosition::COMMANDER1; i <= ArtifactPosition::COMMANDER6; ++i)
		a->possibleSlots[ArtBearer::COMMANDER].push_back(ArtifactPosition(i));
}

bool CArtHandler::legalArtifact(const ArtifactID & id)
{
	auto art = objects[id];
	//assert ( (!art->constituents) || art->constituents->size() ); //artifacts is not combined or has some components

	if(art->constituents)
		return false; //no combo artifacts spawning

	if(art->aClass < CArtifact::ART_TREASURE || art->aClass > CArtifact::ART_RELIC)
		return false; // invalid class

	if(!art->possibleSlots[ArtBearer::HERO].empty())
		return true;

	if(!art->possibleSlots[ArtBearer::CREATURE].empty() && VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_ARTIFACT))
		return true;

	if(!art->possibleSlots[ArtBearer::COMMANDER].empty() && VLC->settings()->getBoolean(EGameSettings::MODULE_COMMANDERS))
		return true;

	return false;
}

void CArtHandler::initAllowedArtifactsList(const std::vector<bool> &allowed)
{
	allowedArtifacts.clear();
	treasures.clear();
	minors.clear();
	majors.clear();
	relics.clear();

	for (ArtifactID i=ArtifactID::SPELLBOOK; i<ArtifactID::ART_SELECTION; i.advance(1))
	{
		//check artifacts allowed on a map
		//TODO: This line will be different when custom map format is implemented
		if (allowed[i] && legalArtifact(i))
			allowedArtifacts.push_back(objects[i]);
	}
	for(ArtifactID i = ArtifactID::ART_SELECTION; i < ArtifactID(static_cast<si32>(objects.size())); i.advance(1)) //try to allow all artifacts added by mods
	{
		if (legalArtifact(ArtifactID(i)))
			allowedArtifacts.push_back(objects[i]);
			 //keep im mind that artifact can be worn by more than one type of bearer
	}
}

std::vector<bool> CArtHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedArtifacts;
	allowedArtifacts.resize(127, true);
	allowedArtifacts.resize(141, false);
	allowedArtifacts.resize(size(), true);
	return allowedArtifacts;
}

void CArtHandler::erasePickedArt(const ArtifactID & id)
{
	CArtifact *art = objects[id];

	std::vector<CArtifact*> * artifactList = nullptr;
	switch(art->aClass)
	{
		case CArtifact::ART_TREASURE:
			artifactList = &treasures;
			break;
		case CArtifact::ART_MINOR:
			artifactList = &minors;
			break;
		case CArtifact::ART_MAJOR:
			artifactList = &majors;
			break;
		case CArtifact::ART_RELIC:
			artifactList = &relics;
			break;
		default:
			logMod->warn("Problem: cannot find list for artifact %s, strange class. (special?)", art->getNameTranslated());
			return;
	}

	if(artifactList->empty())
		fillList(*artifactList, art->aClass);

	auto itr = vstd::find(*artifactList, art);
	if(itr != artifactList->end())
	{
		artifactList->erase(itr);
	}
	else
		logMod->warn("Problem: cannot erase artifact %s from list, it was not present", art->getNameTranslated());
}

void CArtHandler::fillList( std::vector<CArtifact*> &listToBeFilled, CArtifact::EartClass artifactClass )
{
	assert(listToBeFilled.empty());
	for (auto & elem : allowedArtifacts)
	{
		if (elem->aClass == artifactClass)
			listToBeFilled.push_back(elem);
	}
}

void CArtHandler::afterLoadFinalization()
{
	//All artifacts have their id, so we can properly update their bonuses' source ids.
	for(auto &art : objects)
	{
		for(auto &bonus : art->getExportedBonusList())
		{
			assert(art == objects[art->id]);
			assert(bonus->source == Bonus::ARTIFACT);
			bonus->sid = art->id;
		}
	}
	CBonusSystemNode::treeHasChanged();
}

CArtifactInstance::CArtifactInstance()
{
	init();
}

CArtifactInstance::CArtifactInstance( CArtifact *Art)
{
	init();
	setType(Art);
}

void CArtifactInstance::setType( CArtifact *Art )
{
	artType = Art;
	attachTo(*Art);
}

std::string CArtifactInstance::nodeName() const
{
	return "Artifact instance of " + (artType ? artType->getJsonKey() : std::string("uninitialized")) + " type";
}

CArtifactInstance * CArtifactInstance::createScroll(const SpellID & sid)
{
	auto * ret = new CArtifactInstance(VLC->arth->objects[ArtifactID::SPELL_SCROLL]);
	auto b = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::SPELL, Bonus::ARTIFACT_INSTANCE, -1, ArtifactID::SPELL_SCROLL, sid);
	ret->addNewBonus(b);
	return ret;
}

void CArtifactInstance::init()
{
	id = ArtifactInstanceID();
	id = static_cast<ArtifactInstanceID>(ArtifactID::NONE); //to be randomized
	setNodeType(ARTIFACT_INSTANCE);
}

std::string CArtifactInstance::getDescription() const
{
	std::string text = artType->getDescriptionTranslated();
	if (!vstd::contains(text, '{'))
		text = '{' + artType->getNameTranslated() + "}\n\n" + text; //workaround for new artifacts with single name, turns it to H3-style

	if(artType->getId() == ArtifactID::SPELL_SCROLL)
	{
		// we expect scroll description to be like this: This scroll contains the [spell name] spell which is added into your spell book for as long as you carry the scroll.
		// so we want to replace text in [...] with a spell name
		// however other language versions don't have name placeholder at all, so we have to be careful
		SpellID spellID = getScrollSpellID();
		size_t nameStart = text.find_first_of('[');
		size_t nameEnd = text.find_first_of(']', nameStart);
		if(spellID.getNum() >= 0)
		{
			if(nameStart != std::string::npos  &&  nameEnd != std::string::npos)
				text = text.replace(nameStart, nameEnd - nameStart + 1, spellID.toSpell(VLC->spells())->getNameTranslated());
		}
	}
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

void CArtifactInstance::putAt(ArtifactLocation al)
{
	assert(canBePutAt(al));

	al.getHolderArtSet()->setNewArtSlot(al.slot, this, false);
	if(ArtifactUtils::isSlotEquipment(al.slot))
		al.getHolderNode()->attachTo(*this);
}

void CArtifactInstance::removeFrom(ArtifactLocation al)
{
	assert(al.getHolderArtSet()->getArt(al.slot) == this);
	al.getHolderArtSet()->eraseArtSlot(al.slot);
	if(ArtifactUtils::isSlotEquipment(al.slot))
		al.getHolderNode()->detachFrom(*this);
}

bool CArtifactInstance::canBeDisassembled() const
{
	return artType->canBeDisassembled();
}

void CArtifactInstance::move(const ArtifactLocation & src, const ArtifactLocation & dst)
{
	removeFrom(src);
	putAt(dst);
}

CArtifactInstance * CArtifactInstance::createNewArtifactInstance(CArtifact *Art)
{
	if(!Art->constituents)
	{
		auto * ret = new CArtifactInstance(Art);
		if (dynamic_cast<CGrowingArtifact *>(Art))
		{
			auto bonus = std::make_shared<Bonus>();
			bonus->type = Bonus::LEVEL_COUNTER;
			bonus->val = 0;
			ret->addNewBonus (bonus);
		}
		return ret;
	}
	else
	{
		auto * ret = new CCombinedArtifactInstance(Art);
		ret->createConstituents();
		return ret;
	}
}

CArtifactInstance * CArtifactInstance::createNewArtifactInstance(const ArtifactID & aid)
{
	return createNewArtifactInstance(VLC->arth->objects[aid]);
}

CArtifactInstance * CArtifactInstance::createArtifact(CMap * map, const ArtifactID & aid, int spellID)
{
	CArtifactInstance * a = nullptr;
	if(aid >= 0)
	{
		if(spellID < 0)
		{
			a = CArtifactInstance::createNewArtifactInstance(aid);
		}
		else
		{
			a = CArtifactInstance::createScroll(SpellID(spellID));
		}
	}
	else //FIXME: create combined artifact instance for random combined artifacts, just in case
	{
		a = new CArtifactInstance(); //random, empty
	}

	map->addNewArtifactInstance(a);

	//TODO make it nicer
	if(a->artType && (!!a->artType->constituents))
	{
		auto * comb = dynamic_cast<CCombinedArtifactInstance *>(a);
		for(CCombinedArtifactInstance::ConstituentInfo & ci : comb->constituentsInfo)
		{
			map->addNewArtifactInstance(ci.art);
		}
	}
	return a;
}


void CArtifactInstance::deserializationFix()
{
	setType(artType);
}

SpellID CArtifactInstance::getScrollSpellID() const
{
	const auto b = getBonusLocalFirst(Selector::type()(Bonus::SPELL));
	if(!b)
	{
		logMod->warn("Warning: %s doesn't bear any spell!", nodeName());
		return SpellID::NONE;
	}
	return SpellID(b->subtype);
}

bool CArtifactInstance::isPart(const CArtifactInstance *supposedPart) const
{
	return supposedPart == this;
}

CCombinedArtifactInstance::CCombinedArtifactInstance(CArtifact *Art)
	: CArtifactInstance(Art) //TODO: seems unused, but need to be written
{
}

void CCombinedArtifactInstance::createConstituents()
{
	assert(artType);
	assert(artType->constituents);

	for(const CArtifact * art : *artType->constituents)
	{
		addAsConstituent(CArtifactInstance::createNewArtifactInstance(art->getId()), ArtifactPosition::PRE_FIRST);
	}
}

void CCombinedArtifactInstance::addAsConstituent(CArtifactInstance * art, const ArtifactPosition & slot)
{
	assert(vstd::contains_if(*artType->constituents, [=](const CArtifact * constituent){
		return constituent->getId() == art->artType->getId();
	}));
	assert(art->getParentNodes().size() == 1  &&  art->getParentNodes().front() == art->artType);
	constituentsInfo.emplace_back(art, slot);
	attachTo(*art);
}

void CCombinedArtifactInstance::putAt(ArtifactLocation al)
{
	if(al.slot == ArtifactPosition::TRANSITION_POS)
	{
		CArtifactInstance::putAt(al);
	}
	else if(ArtifactUtils::isSlotBackpack(al.slot))
	{
		CArtifactInstance::putAt(al);
		for(ConstituentInfo &ci : constituentsInfo)
			ci.slot = ArtifactPosition::PRE_FIRST;
	}
	else
	{
		CArtifactInstance *mainConstituent = figureMainConstituent(al); //it'll be replaced with combined artifact, not a lock
		CArtifactInstance::putAt(al); //puts combined art (this)

		for(ConstituentInfo & ci : constituentsInfo)
		{
			if(ci.art != mainConstituent)
			{
				const ArtifactLocation suggestedPos(al.artHolder, ci.slot);
				const bool inActiveSlot = vstd::isbetween(ci.slot, 0, GameConstants::BACKPACK_START);
				const bool suggestedPosValid = ci.art->canBePutAt(suggestedPos);

				if(!(inActiveSlot && suggestedPosValid)) //there is a valid suggestion where to place lock
					ci.slot = ArtifactUtils::getArtAnyPosition(al.getHolderArtSet(), ci.art->getTypeId());

				assert(ArtifactUtils::isSlotEquipment(ci.slot));
				al.getHolderArtSet()->setNewArtSlot(ci.slot, ci.art, true); //sets as lock
			}
			else
			{
				ci.slot = ArtifactPosition::PRE_FIRST;
			}
		}
	}
}

void CCombinedArtifactInstance::removeFrom(ArtifactLocation al)
{
	if(ArtifactUtils::isSlotBackpack(al.slot) || al.slot == ArtifactPosition::TRANSITION_POS)
	{
		CArtifactInstance::removeFrom(al);
	}
	else
	{
		for(ConstituentInfo &ci : constituentsInfo)
		{
			if(ci.slot >= 0)
			{
				al.getHolderArtSet()->eraseArtSlot(ci.slot);
				ci.slot = ArtifactPosition::PRE_FIRST;
			}
			else
			{
				//main constituent
				CArtifactInstance::removeFrom(al);
			}
		}
	}
}

CArtifactInstance * CCombinedArtifactInstance::figureMainConstituent(const ArtifactLocation & al)
{
	CArtifactInstance *mainConstituent = nullptr; //it'll be replaced with combined artifact, not a lock
	for(ConstituentInfo &ci : constituentsInfo)
		if(ci.slot == al.slot)
			mainConstituent = ci.art;

	if(!mainConstituent)
	{
		for(ConstituentInfo &ci : constituentsInfo)
		{
			if(vstd::contains(ci.art->artType->possibleSlots[al.getHolderArtSet()->bearerType()], al.slot))
			{
				mainConstituent = ci.art;
			}
		}
	}

	return mainConstituent;
}

void CCombinedArtifactInstance::deserializationFix()
{
	for(ConstituentInfo &ci : constituentsInfo)
		attachTo(*ci.art);
}

bool CCombinedArtifactInstance::isPart(const CArtifactInstance *supposedPart) const
{
	bool me = CArtifactInstance::isPart(supposedPart);
	if(me)
		return true;

	//check for constituents
	for(const ConstituentInfo &constituent : constituentsInfo)
		if(constituent.art == supposedPart)
			return true;

	return false;
}

CCombinedArtifactInstance::ConstituentInfo::ConstituentInfo(CArtifactInstance * Art, const ArtifactPosition & Slot):
	art(Art),
	slot(Slot)
{
}

bool CCombinedArtifactInstance::ConstituentInfo::operator==(const ConstituentInfo &rhs) const
{
	return art == rhs.art && slot == rhs.slot;
}

CArtifactSet::~CArtifactSet() = default;

const CArtifactInstance * CArtifactSet::getArt(const ArtifactPosition & pos, bool excludeLocked) const
{
	if(const ArtSlotInfo *si = getSlot(pos))
	{
		if(si->artifact && (!excludeLocked || !si->locked))
			return si->artifact;
	}

	return nullptr;
}

CArtifactInstance * CArtifactSet::getArt(const ArtifactPosition & pos, bool excludeLocked)
{
	return const_cast<CArtifactInstance*>((const_cast<const CArtifactSet*>(this))->getArt(pos, excludeLocked));
}

ArtifactPosition CArtifactSet::getArtPos(const ArtifactID & aid, bool onlyWorn, bool allowLocked) const
{
	const auto result = getAllArtPositions(aid, onlyWorn, allowLocked, false);
	return result.empty() ? ArtifactPosition{ArtifactPosition::PRE_FIRST} : result[0];
}

ArtifactPosition CArtifactSet::getArtBackpackPos(const ArtifactID & aid) const
{
	const auto result = getBackpackArtPositions(aid);
	return result.empty() ? ArtifactPosition{ArtifactPosition::PRE_FIRST} : result[0];
}

std::vector<ArtifactPosition> CArtifactSet::getAllArtPositions(const ArtifactID & aid, bool onlyWorn, bool allowLocked, bool getAll) const
{
	std::vector<ArtifactPosition> result;
	for(const auto & slotInfo : artifactsWorn)
		if(slotInfo.second.artifact->getTypeId() == aid && (allowLocked || !slotInfo.second.locked))
			result.push_back(slotInfo.first);

	if(onlyWorn)
		return result;
	if(!getAll && !result.empty())
		return result;

	auto backpackPositions = getBackpackArtPositions(aid);
	result.insert(result.end(), backpackPositions.begin(), backpackPositions.end());
	return result;
}

std::vector<ArtifactPosition> CArtifactSet::getBackpackArtPositions(const ArtifactID & aid) const
{
	std::vector<ArtifactPosition> result;

	si32 backpackPosition = GameConstants::BACKPACK_START;
	for(const auto & artInfo : artifactsInBackpack)
	{
		const auto * art = artInfo.getArt();
		if(art && art->artType->getId() == aid)
			result.emplace_back(backpackPosition);
		backpackPosition++;
	}
	return result;
}

ArtifactPosition CArtifactSet::getArtPos(const CArtifactInstance *art) const
{
	for(auto i : artifactsWorn)
		if(i.second.artifact == art)
			return i.first;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact == art)
			return ArtifactPosition(GameConstants::BACKPACK_START + i);

	return ArtifactPosition::PRE_FIRST;
}

const CArtifactInstance * CArtifactSet::getArtByInstanceId(const ArtifactInstanceID & artInstId) const
{
	for(auto i : artifactsWorn)
		if(i.second.artifact->id == artInstId)
			return i.second.artifact;

	for(auto i : artifactsInBackpack)
		if(i.artifact->id == artInstId)
			return i.artifact;

	return nullptr;
}

bool CArtifactSet::hasArt(const ArtifactID & aid, bool onlyWorn, bool searchBackpackAssemblies, bool allowLocked) const
{
	return getArtPosCount(aid, onlyWorn, searchBackpackAssemblies, allowLocked) > 0;
}

bool CArtifactSet::hasArtBackpack(const ArtifactID & aid) const
{
	return !getBackpackArtPositions(aid).empty();
}

unsigned CArtifactSet::getArtPosCount(const ArtifactID & aid, bool onlyWorn, bool searchBackpackAssemblies, bool allowLocked) const
{
	const auto allPositions = getAllArtPositions(aid, onlyWorn, allowLocked, true);
	if(!allPositions.empty())
		return allPositions.size();

	if(searchBackpackAssemblies && getHiddenArt(aid))
		return 1;

	return 0;
}

std::pair<const CCombinedArtifactInstance *, const CArtifactInstance *> CArtifactSet::searchForConstituent(const ArtifactID & aid) const
{
	for(const auto & slot : artifactsInBackpack)
	{
		auto art = slot.artifact;
		if(art->canBeDisassembled())
		{
			auto * ass = dynamic_cast<CCombinedArtifactInstance *>(art.get());
			for(auto& ci : ass->constituentsInfo)
			{
				if(ci.art->getTypeId() == aid)
				{
					return {ass, ci.art};
				}
			}
		}
	}
	return {nullptr, nullptr};
}

const CArtifactInstance * CArtifactSet::getHiddenArt(const ArtifactID & aid) const
{
	return searchForConstituent(aid).second;
}

const CCombinedArtifactInstance * CArtifactSet::getAssemblyByConstituent(const ArtifactID & aid) const
{
	return searchForConstituent(aid).first;
}

const ArtSlotInfo * CArtifactSet::getSlot(const ArtifactPosition & pos) const
{
	if(pos == ArtifactPosition::TRANSITION_POS)
	{
		// Always add to the end. Always take from the beginning.
		if(artifactsTransitionPos.empty())
			return nullptr;
		else
			return &(*artifactsTransitionPos.begin());
	}
	if(vstd::contains(artifactsWorn, pos))
		return &artifactsWorn.at(pos);
	if(pos >= ArtifactPosition::AFTER_LAST )
	{
		int backpackPos = (int)pos - GameConstants::BACKPACK_START;
		if(backpackPos < 0 || backpackPos >= artifactsInBackpack.size())
			return nullptr;
		else
			return &artifactsInBackpack[backpackPos];
	}

	return nullptr;
}

bool CArtifactSet::isPositionFree(const ArtifactPosition & pos, bool onlyLockCheck) const
{
	if(const ArtSlotInfo *s = getSlot(pos))
		return (onlyLockCheck || !s->artifact) && !s->locked;

	return true; //no slot means not used
}

ArtSlotInfo & CArtifactSet::retrieveNewArtSlot(const ArtifactPosition & slot)
{
	assert(!vstd::contains(artifactsWorn, slot));

	if(slot == ArtifactPosition::TRANSITION_POS)
	{
		// Always add to the end. Always take from the beginning.
		artifactsTransitionPos.emplace_back();
		return artifactsTransitionPos.back();
	}
	if(!ArtifactUtils::isSlotBackpack(slot))
		return artifactsWorn[slot];

	ArtSlotInfo newSlot;
	size_t index  = slot - GameConstants::BACKPACK_START;
	auto position = artifactsInBackpack.begin() + index;
	auto inserted = artifactsInBackpack.insert(position, newSlot);

	return *inserted;
}

void CArtifactSet::setNewArtSlot(const ArtifactPosition & slot, CArtifactInstance * art, bool locked)
{
	ArtSlotInfo & asi = retrieveNewArtSlot(slot);
	asi.artifact = art;
	asi.locked = locked;
}

void CArtifactSet::eraseArtSlot(const ArtifactPosition & slot)
{
	if(slot == ArtifactPosition::TRANSITION_POS)
	{
		assert(!artifactsTransitionPos.empty());
		artifactsTransitionPos.erase(artifactsTransitionPos.begin());
	}
	else if(ArtifactUtils::isSlotBackpack(slot))
	{
		auto backpackSlot = ArtifactPosition(slot - GameConstants::BACKPACK_START);

		assert(artifactsInBackpack.begin() + backpackSlot < artifactsInBackpack.end());
		artifactsInBackpack.erase(artifactsInBackpack.begin() + backpackSlot);
	}
	else
	{
		artifactsWorn.erase(slot);
	}
}

void CArtifactSet::artDeserializationFix(CBonusSystemNode *node)
{
	for(auto & elem : artifactsWorn)
		if(elem.second.artifact && !elem.second.locked)
			node->attachTo(*elem.second.artifact);
}

void CArtifactSet::serializeJsonArtifacts(JsonSerializeFormat & handler, const std::string & fieldName, CMap * map)
{
	//todo: creature and commander artifacts
	if(handler.saving && artifactsInBackpack.empty() && artifactsWorn.empty())
		return;

	if(!handler.saving)
	{
		assert(map);
		artifactsInBackpack.clear();
		artifactsWorn.clear();
	}

	auto s = handler.enterStruct(fieldName);

	switch(bearerType())
	{
	case ArtBearer::HERO:
		serializeJsonHero(handler, map);
		break;
	case ArtBearer::CREATURE:
		serializeJsonCreature(handler, map);
		break;
	case ArtBearer::COMMANDER:
		serializeJsonCommander(handler, map);
		break;
	default:
		assert(false);
		break;
	}
}

void CArtifactSet::serializeJsonHero(JsonSerializeFormat & handler, CMap * map)
{
	for(ArtifactPosition ap = ArtifactPosition::HEAD; ap < ArtifactPosition::AFTER_LAST; ap.advance(1))
	{
		serializeJsonSlot(handler, ap, map);
	}

	std::vector<ArtifactID> backpackTemp;

	if(handler.saving)
	{
		backpackTemp.reserve(artifactsInBackpack.size());
		for(const ArtSlotInfo & info : artifactsInBackpack)
			backpackTemp.push_back(info.artifact->getTypeId());
	}
	handler.serializeIdArray(NArtifactPosition::backpack, backpackTemp);
	if(!handler.saving)
	{
		for(const ArtifactID & artifactID : backpackTemp)
		{
			auto * artifact = CArtifactInstance::createArtifact(map, artifactID.toEnum());
			auto slot = ArtifactPosition(GameConstants::BACKPACK_START + (si32)artifactsInBackpack.size());
			if(artifact->artType->canBePutAt(this, slot))
				putArtifact(slot, artifact);
		}
	}
}

void CArtifactSet::serializeJsonCreature(JsonSerializeFormat & handler, CMap * map)
{
	logGlobal->error("CArtifactSet::serializeJsonCreature not implemented");
}

void CArtifactSet::serializeJsonCommander(JsonSerializeFormat & handler, CMap * map)
{
	logGlobal->error("CArtifactSet::serializeJsonCommander not implemented");
}

void CArtifactSet::serializeJsonSlot(JsonSerializeFormat & handler, const ArtifactPosition & slot, CMap * map)
{
	ArtifactID artifactID;

	if(handler.saving)
	{
		const ArtSlotInfo * info = getSlot(slot);

		if(info != nullptr && !info->locked)
		{
			artifactID = info->artifact->getTypeId();
			handler.serializeId(NArtifactPosition::namesHero[slot.num], artifactID, ArtifactID::NONE);
		}
	}
	else
	{
		handler.serializeId(NArtifactPosition::namesHero[slot.num], artifactID, ArtifactID::NONE);

		if(artifactID != ArtifactID::NONE)
		{
			auto * artifact = CArtifactInstance::createArtifact(map, artifactID.toEnum());

			if(artifact->artType->canBePutAt(this, slot))
			{
				putArtifact(slot, artifact);
			}
			else
			{
				logGlobal->debug("Artifact can't be put at the specified location."); //TODO add more debugging information
			}
		}
	}
}

CArtifactFittingSet::CArtifactFittingSet(ArtBearer::ArtBearer Bearer):
	Bearer(Bearer)
{
}

void CArtifactFittingSet::putArtifact(ArtifactPosition pos, CArtifactInstance * art)
{
	if(art && art->canBeDisassembled() && ArtifactUtils::isSlotEquipment(pos))
	{
		for(auto & part : dynamic_cast<CCombinedArtifactInstance*>(art)->constituentsInfo)
		{
			const auto slot = ArtifactUtils::getArtAnyPosition(this, part.art->getTypeId());
			assert(slot != ArtifactPosition::PRE_FIRST);
			// For the ArtFittingSet is no needed to do figureMainConstituent, just lock slots
			this->setNewArtSlot(slot, part.art, true);
		}
	}
	else
	{
		this->setNewArtSlot(pos, art, false);
	}
}

void CArtifactFittingSet::removeArtifact(ArtifactPosition pos)
{
	// Removes the art from the CartifactSet, but does not remove it from art->constituentsInfo.
	// removeArtifact is for CArtifactFittingSet only. Do not move it to the parent class.

	auto art = getArt(pos);
	if(art == nullptr)
		return;
	if(art->canBeDisassembled())
	{
		auto combinedArt = dynamic_cast<CCombinedArtifactInstance*>(art);
		for(const auto & part : combinedArt->constituentsInfo)
		{
			if(ArtifactUtils::isSlotEquipment(part.slot))
				eraseArtSlot(part.slot);
		}
	}
	eraseArtSlot(pos);
}

ArtBearer::ArtBearer CArtifactFittingSet::bearerType() const
{
	return this->Bearer;
}

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtAnyPosition(const CArtifactSet * target, const ArtifactID & aid)
{
	const auto * art = aid.toArtifact();
	for(const auto & slot : art->possibleSlots.at(target->bearerType()))
	{
		if(art->canBePutAt(target, slot))
			return slot;
	}
	return getArtBackpackPosition(target, aid);
}

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtBackpackPosition(const CArtifactSet * target, const ArtifactID & aid)
{
	const auto * art = aid.toArtifact();
	if(art->canBePutAt(target, GameConstants::BACKPACK_START))
	{
		return GameConstants::BACKPACK_START;
	}
	return ArtifactPosition::PRE_FIRST;
}

DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & ArtifactUtils::unmovableSlots()
{
	static const std::vector<ArtifactPosition::EArtifactPosition> positions =
	{
		ArtifactPosition::SPELLBOOK,
		ArtifactPosition::MACH4
	};

	return positions;
}

DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & ArtifactUtils::constituentWornSlots()
{
	static const std::vector<ArtifactPosition::EArtifactPosition> positions =
	{
		ArtifactPosition::HEAD,
		ArtifactPosition::SHOULDERS,
		ArtifactPosition::NECK,
		ArtifactPosition::RIGHT_HAND,
		ArtifactPosition::LEFT_HAND,
		ArtifactPosition::TORSO,
		ArtifactPosition::RIGHT_RING,
		ArtifactPosition::LEFT_RING,
		ArtifactPosition::FEET,
		ArtifactPosition::MISC1,
		ArtifactPosition::MISC2,
		ArtifactPosition::MISC3,
		ArtifactPosition::MISC4,
		ArtifactPosition::MISC5,
	};

	return positions;
}

DLL_LINKAGE bool ArtifactUtils::isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot)
{
	return slot.second.artifact
		&& !slot.second.locked
		&& !vstd::contains(unmovableSlots(), slot.first);
}

DLL_LINKAGE bool ArtifactUtils::checkSpellbookIsNeeded(const CGHeroInstance * heroPtr, const ArtifactID & artID, const ArtifactPosition & slot)
{
	// TODO what'll happen if Titan's thunder is equipped by pickin git up or the start of game?
	// Titan's Thunder creates new spellbook on equip
	if(artID == ArtifactID::TITANS_THUNDER && slot == ArtifactPosition::RIGHT_HAND)
	{
		if(heroPtr)
		{
			if(heroPtr && !heroPtr->hasSpellbook())
				return true;
		}
	}
	return false;
}

DLL_LINKAGE bool ArtifactUtils::isSlotBackpack(const ArtifactPosition & slot)
{
	return slot >= GameConstants::BACKPACK_START;
}

DLL_LINKAGE bool ArtifactUtils::isSlotEquipment(const ArtifactPosition & slot)
{
	return slot < GameConstants::BACKPACK_START && slot >= 0;
}

DLL_LINKAGE bool ArtifactUtils::isBackpackFreeSlots(const CArtifactSet * target, const size_t reqSlots)
{
	const auto backpackCap = VLC->settings()->getInteger(EGameSettings::HEROES_BACKPACK_CAP);
	if(backpackCap < 0)
		return true;
	else
		return target->artifactsInBackpack.size() + reqSlots <= backpackCap;
}

DLL_LINKAGE std::vector<const CArtifact*> ArtifactUtils::assemblyPossibilities(
	const CArtifactSet * artSet, const ArtifactID & aid, bool equipped)
{
	std::vector<const CArtifact*> arts;
	const auto * art = aid.toArtifact();
	if(art->canBeDisassembled())
		return arts;

	for(const auto artifact : art->constituentOf)
	{
		assert(artifact->constituents);
		bool possible = true;

		for(const auto constituent : *artifact->constituents) //check if all constituents are available
		{
			if(equipped)
			{
				// Search for equipped arts
				if(!artSet->hasArt(constituent->getId(), true, false, false))
				{
					possible = false;
					break;
				}
			}
			else
			{
				// Search in backpack
				if(!artSet->hasArtBackpack(constituent->getId()))
				{
					possible = false;
					break;
				}
			}
		}
		if(possible)
			arts.push_back(artifact);
	}
	return arts;
}

VCMI_LIB_NAMESPACE_END
