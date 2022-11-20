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

const std::string & CArtifact::getName() const
{
	return name;
}

const std::string & CArtifact::getJsonKey() const
{
	return identifier;
}

void CArtifact::registerIcons(const IconRegistar & cb) const
{
	cb(getIconIndex(), "ARTIFACT", image);
	cb(getIconIndex(), "ARTIFACTLARGE", large);
}

ArtifactID CArtifact::getId() const
{
	return id;
}

const IBonusBearer * CArtifact::accessBonuses() const
{
	return this;
}

const std::string & CArtifact::getDescription() const
{
	return description;
}
const std::string & CArtifact::getEventText() const
{
	return eventText;
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

CArtifact::CArtifact()
{
	setNodeType(ARTIFACT);
	possibleSlots[ArtBearer::HERO]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::CREATURE]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::COMMANDER];
	iconIndex = ArtifactID::NONE;
	price = 0;
	aClass = ART_SPECIAL;
}

CArtifact::~CArtifact()
{
}

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
	return "Artifact: " + getName();
}

void CArtifact::addNewBonus(const std::shared_ptr<Bonus>& b)
{
	b->source = Bonus::ARTIFACT;
	b->duration = Bonus::PERMANENT;
	b->description = name;
	CBonusSystemNode::addNewBonus(b);
}

void CArtifact::updateFrom(const JsonNode& data)
{
	//TODO:CArtifact::updateFrom
}

void CArtifact::serializeJson(JsonSerializeFormat & handler)
{

}

void CArtifact::fillWarMachine()
{
	switch(id)
	{
	case ArtifactID::CATAPULT:
		warMachine = CreatureID::CATAPULT;
		break;
	case ArtifactID::BALLISTA:
		warMachine = CreatureID::BALLISTA;
		break;
	case ArtifactID::FIRST_AID_TENT:
		warMachine = CreatureID::FIRST_AID_TENT;
		break;
	case ArtifactID::AMMO_CART:
		warMachine = CreatureID::AMMO_CART;
		break;
	default:
		warMachine = CreatureID::NONE;
		break;
	}
}

void CGrowingArtifact::levelUpArtifact (CArtifactInstance * art)
{
	auto b = std::make_shared<Bonus>();
	b->type = Bonus::LEVEL_COUNTER;
	b->val = 1;
	b->duration = Bonus::COMMANDER_KILLED;
	art->accumulateBonus(b);

	for (auto bonus : bonusesPerLevel)
	{
		if (art->valOfBonuses(Bonus::LEVEL_COUNTER) % bonus.first == 0) //every n levels
		{
			art->accumulateBonus(std::make_shared<Bonus>(bonus.second));
		}
	}
	for (auto bonus : thresholdBonuses)
	{
		if (art->valOfBonuses(Bonus::LEVEL_COUNTER) == bonus.first) //every n levels
		{
			art->addNewBonus(std::make_shared<Bonus>(bonus.second));
		}
	}
}

CArtHandler::CArtHandler()
{
}

CArtHandler::~CArtHandler() = default;

std::vector<JsonNode> CArtHandler::loadLegacyData(size_t dataSize)
{
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

		for(auto & artSlot : artSlots)
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
	auto object = loadFromJson(scope, data, normalizeIdentifier(scope, "core", name), objects.size());

	object->iconIndex = object->getIndex() + 5;

	objects.push_back(object);

	registerObject(scope, "artifact", name, object->id);
}

void CArtHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(scope, data, normalizeIdentifier(scope, "core", name), index);

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
	CArtifact * art;

	if(!VLC->modh->modules.COMMANDERS || node["growing"].isNull())
	{
		art = new CArtifact();
	}
	else
	{
		auto  growing = new CGrowingArtifact();
		loadGrowingArt(growing, node);
		art = growing;
	}
	art->id = ArtifactID(index);
	art->identifier = identifier;
	const JsonNode & text = node["text"];
	art->name        = text["name"].String();
	art->description = text["description"].String();
	art->eventText   = text["event"].String();

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

	for(auto b : node["bonuses"].Vector())
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

		VLC->objtypeh->loadSubObject(art->getJsonKey(), conf, Obj::ARTIFACT, art->getIndex());

		if(!art->advMapDef.empty())
		{
			JsonNode templ;
			templ.setMeta(scope);
			templ["animation"].String() = art->advMapDef;

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

ArtifactPosition CArtHandler::stringToSlot(std::string slotName)
{
#define ART_POS(x) { #x, ArtifactPosition::x },
	static const std::map<std::string, ArtifactPosition> artifactPositionMap = { ART_POS_LIST };
#undef ART_POS
	auto it = artifactPositionMap.find (slotName);
	if (it != artifactPositionMap.end())
		return it->second;

	logMod->warn("Warning! Artifact slot %s not recognized!", slotName);
	return ArtifactPosition::PRE_FIRST;
}

void CArtHandler::addSlot(CArtifact * art, const std::string & slotID)
{
	static const std::vector<ArtifactPosition> miscSlots =
	{
		ArtifactPosition::MISC1, ArtifactPosition::MISC2, ArtifactPosition::MISC3, ArtifactPosition::MISC4, ArtifactPosition::MISC5
	};

	static const std::vector<ArtifactPosition> ringSlots =
	{
		ArtifactPosition::LEFT_RING, ArtifactPosition::RIGHT_RING
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
		auto slot = stringToSlot(slotID);
		if (slot != ArtifactPosition::PRE_FIRST)
			art->possibleSlots[ArtBearer::HERO].push_back (slot);
	}
}

void CArtHandler::loadSlots(CArtifact * art, const JsonNode & node)
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
	}
}

CArtifact::EartClass CArtHandler::stringToClass(std::string className)
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

void CArtHandler::loadClass(CArtifact * art, const JsonNode & node)
{
	art->aClass = stringToClass(node["class"].String());
}

void CArtHandler::loadType(CArtifact * art, const JsonNode & node)
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
		art->constituents.reset(new std::vector<CArtifact *>());
		for (auto component : node["components"].Vector())
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

void CArtHandler::loadGrowingArt(CGrowingArtifact * art, const JsonNode & node)
{
	for (auto b : node["growing"]["bonusesPerLevel"].Vector())
	{
		art->bonusesPerLevel.push_back(std::pair <ui16, Bonus>((ui16)b["level"].Float(), Bonus()));
		JsonUtils::parseBonus(b["bonus"], &art->bonusesPerLevel.back().second);
	}
	for (auto b : node["growing"]["thresholdBonuses"].Vector())
	{
		art->thresholdBonuses.push_back(std::pair <ui16, Bonus>((ui16)b["level"].Float(), Bonus()));
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
				out.push_back(art);
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
		if (!out.size()) //no artifact of specified rarity, we need to take another one
		{
			getAllowedArts (out, &treasures, CArtifact::ART_TREASURE);
			getAllowedArts (out, &minors, CArtifact::ART_MINOR);
			getAllowedArts (out, &majors, CArtifact::ART_MAJOR);
			getAllowedArts (out, &relics, CArtifact::ART_RELIC);
		}
		if (!out.size()) //no arts are available at all
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
	return pickRandomArtifact(rand, 0xff, accepts);
}

ArtifactID CArtHandler::pickRandomArtifact(CRandomGenerator & rand, int flags)
{
	return pickRandomArtifact(rand, flags, [](ArtifactID){ return true;});
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

bool CArtHandler::legalArtifact(ArtifactID id)
{
	auto art = objects[id];
	//assert ( (!art->constituents) || art->constituents->size() ); //artifacts is not combined or has some components
	return ((art->possibleSlots[ArtBearer::HERO].size() ||
		(art->possibleSlots[ArtBearer::COMMANDER].size() && VLC->modh->modules.COMMANDERS) ||
		(art->possibleSlots[ArtBearer::CREATURE].size() && VLC->modh->modules.STACK_ARTIFACT)) &&
		!(art->constituents) && //no combo artifacts spawning
		art->aClass >= CArtifact::ART_TREASURE &&
		art->aClass <= CArtifact::ART_RELIC);
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
	for (ArtifactID i = ArtifactID::ART_SELECTION; i<ArtifactID((si32)objects.size()); i.advance(1)) //try to allow all artifacts added by mods
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

void CArtHandler::erasePickedArt(ArtifactID id)
{
	CArtifact *art = objects[id];

	if(auto artifactList = listFromClass(art->aClass))
	{
		if(artifactList->empty())
			fillList(*artifactList, art->aClass);

		auto itr = vstd::find(*artifactList, art);
		if(itr != artifactList->end())
		{
			artifactList->erase(itr);
		}
		else
			logMod->warn("Problem: cannot erase artifact %s from list, it was not present", art->getName());

	}
	else
		logMod->warn("Problem: cannot find list for artifact %s, strange class. (special?)", art->getName());
}

boost::optional<std::vector<CArtifact*>&> CArtHandler::listFromClass( CArtifact::EartClass artifactClass )
{
	switch(artifactClass)
	{
	case CArtifact::ART_TREASURE:
		return treasures;
	case CArtifact::ART_MINOR:
		return minors;
	case CArtifact::ART_MAJOR:
		return majors;
	case CArtifact::ART_RELIC:
		return relics;
	default: //special artifacts should not be erased
		return boost::optional<std::vector<CArtifact*>&>();
	}
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
	return "Artifact instance of " + (artType ? artType->getName() : std::string("uninitialized")) + " type";
}

CArtifactInstance *CArtifactInstance::createScroll(SpellID sid)
{
	auto ret = new CArtifactInstance(VLC->arth->objects[ArtifactID::SPELL_SCROLL]);
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

std::string CArtifactInstance::getEffectiveDescription(const CGHeroInstance * hero) const
{
	std::string text = artType->getDescription();
	if (!vstd::contains(text, '{'))
		text = '{' + artType->getName() + "}\n\n" + text; //workaround for new artifacts with single name, turns it to H3-style

	if(artType->id == ArtifactID::SPELL_SCROLL)
	{
		// we expect scroll description to be like this: This scroll contains the [spell name] spell which is added into your spell book for as long as you carry the scroll.
		// so we want to replace text in [...] with a spell name
		// however other language versions don't have name placeholder at all, so we have to be careful
		SpellID spellID = getGivenSpellID();
		size_t nameStart = text.find_first_of('[');
		size_t nameEnd = text.find_first_of(']', nameStart);
		if(spellID.getNum() >= 0)
		{
			if(nameStart != std::string::npos  &&  nameEnd != std::string::npos)
				text = text.replace(nameStart, nameEnd - nameStart + 1, spellID.toSpell(VLC->spells())->getName());
		}
	}
	else if(hero && artType->constituentOf.size()) //display info about set
	{
		std::string artList;
		auto combinedArt = artType->constituentOf[0];
		text += "\n\n";
		text += "{" + combinedArt->getName() + "}";
		int wornArtifacts = 0;
		for(auto a : *combinedArt->constituents) //TODO: can the artifact be a part of more than one set?
		{
			artList += "\n" + a->getName();
			if (hero->hasArt(a->id, true))
				wornArtifacts++;
		}
		text += " (" + boost::str(boost::format("%d") % wornArtifacts) +  " / " +
			boost::str(boost::format("%d") % combinedArt->constituents->size()) + ")" + artList;
		//TODO: fancy colors and fonts for this text
	}

	return text;
}

ArtifactPosition CArtifactInstance::firstAvailableSlot(const CArtifactSet *h) const
{
	for(auto slot : artType->possibleSlots.at(h->bearerType()))
	{
		if(canBePutAt(h, slot)) //if(artType->fitsAt(h->artifWorn, slot))
		{
			//we've found a free suitable slot.
			return slot;
		}
	}

	//if haven't find proper slot, use backpack
	return firstBackpackSlot(h);
}

ArtifactPosition CArtifactInstance::firstBackpackSlot(const CArtifactSet *h) const
{
	if(!artType->isBig()) //discard big artifact
		return ArtifactPosition(
			GameConstants::BACKPACK_START + (si32)h->artifactsInBackpack.size());

	return ArtifactPosition::PRE_FIRST;
}

bool CArtifactInstance::canBePutAt(const ArtifactLocation & al, bool assumeDestRemoved) const
{
	return canBePutAt(al.getHolderArtSet(), al.slot, assumeDestRemoved);
}

bool CArtifactInstance::canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved) const
{
	if(slot >= GameConstants::BACKPACK_START)
	{
		if(artType->isBig())
			return false;

		//TODO backpack limit
		return true;
	}

 	auto possibleSlots = artType->possibleSlots.find(artSet->bearerType());
 	if(possibleSlots == artType->possibleSlots.end())
 	{
		logMod->warn("Warning: artifact %s doesn't have defined allowed slots for bearer of type %s", artType->getName(), artSet->bearerType());
		return false;
	}

	if(!vstd::contains(possibleSlots->second, slot))
		return false;

	return artSet->isPositionFree(slot, assumeDestRemoved);
}

void CArtifactInstance::putAt(ArtifactLocation al)
{
	assert(canBePutAt(al));

	al.getHolderArtSet()->setNewArtSlot(al.slot, this, false);
	if(al.slot < GameConstants::BACKPACK_START)
		al.getHolderNode()->attachTo(*this);
}

void CArtifactInstance::removeFrom(ArtifactLocation al)
{
	assert(al.getHolderArtSet()->getArt(al.slot) == this);
	al.getHolderArtSet()->eraseArtSlot(al.slot);
	if(al.slot < GameConstants::BACKPACK_START)
		al.getHolderNode()->detachFrom(*this);

	//TODO delete me?
}

bool CArtifactInstance::canBeDisassembled() const
{
	return bool(artType->constituents);
}

std::vector<const CArtifact *> CArtifactInstance::assemblyPossibilities(const CArtifactSet * h, bool equipped) const
{
	std::vector<const CArtifact *> ret;
	if(artType->constituents) //combined artifact already: no combining of combined artifacts... for now.
		return ret;

	for(const auto * artifact : artType->constituentOf)
	{
		assert(artifact->constituents);
		bool possible = true;

		for(const auto * constituent : *artifact->constituents) //check if all constituents are available
		{
			if(equipped)
			{
				// Search for equipped arts
				if (!h->hasArt(constituent->id, true, false, false))
				{
					possible = false;
					break;
				}
			}
			else
			{
				// Search in backpack
				if(!h->hasArtBackpack(constituent->id))
				{
					possible = false;
					break;
				}
			}
		}

		if(possible)
			ret.push_back(artifact);
	}

	return ret;
}

void CArtifactInstance::move(ArtifactLocation src, ArtifactLocation dst)
{
	removeFrom(src);
	putAt(dst);
}

CArtifactInstance * CArtifactInstance::createNewArtifactInstance(CArtifact *Art)
{
	if(!Art->constituents)
	{
		auto  ret = new CArtifactInstance(Art);
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
		auto  ret = new CCombinedArtifactInstance(Art);
		ret->createConstituents();
		return ret;
	}
}

CArtifactInstance * CArtifactInstance::createNewArtifactInstance(int aid)
{
	return createNewArtifactInstance(VLC->arth->objects[aid]);
}

CArtifactInstance * CArtifactInstance::createArtifact(CMap * map, int aid, int spellID)
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
		CCombinedArtifactInstance * comb = dynamic_cast<CCombinedArtifactInstance *>(a);
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

SpellID CArtifactInstance::getGivenSpellID() const
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

bool CCombinedArtifactInstance::canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved) const
{
	bool canMainArtifactBePlaced = CArtifactInstance::canBePutAt(artSet, slot, assumeDestRemoved);
	if(!canMainArtifactBePlaced)
		return false; //no is no...
	if(slot >= GameConstants::BACKPACK_START)
		return true; //we can always remove combined art to the backapck


	assert(artType->constituents);
	std::vector<ConstituentInfo> constituentsToBePlaced = constituentsInfo; //we'll remove constituents from that list, as we find a suitable slot for them

	//it may be that we picked a combined artifact in hero screen (though technically it's still there) to move it
	//so we remove from the list all constituents that are already present on dst hero in the form of locks
	for(const ConstituentInfo &constituent : constituentsInfo)
	{
		if(constituent.art == artSet->getArt(constituent.slot, false)) //no need to worry about locked constituent
			constituentsToBePlaced -= constituent;
	}

	//we iterate over all active slots and check if constituents fits them
	for (int i = 0; i < GameConstants::BACKPACK_START; i++)
	{
		for(auto art = constituentsToBePlaced.begin(); art != constituentsToBePlaced.end(); art++)
		{
			if(art->art->canBePutAt(artSet, ArtifactPosition(i), i == slot)) // i == al.slot because we can remove already worn artifact only from that slot  that is our main destination
			{
				constituentsToBePlaced.erase(art);
				break;
			}
		}
	}

	return constituentsToBePlaced.empty();
}

bool CCombinedArtifactInstance::canBeDisassembled() const
{
	return true;
}

CCombinedArtifactInstance::CCombinedArtifactInstance(CArtifact *Art)
	: CArtifactInstance(Art) //TODO: seems unused, but need to be written
{
}

CCombinedArtifactInstance::CCombinedArtifactInstance()
{
}

void CCombinedArtifactInstance::createConstituents()
{
	assert(artType);
	assert(artType->constituents);

	for(const CArtifact * art : *artType->constituents)
	{
		addAsConstituent(CArtifactInstance::createNewArtifactInstance(art->id), ArtifactPosition::PRE_FIRST);
	}
}

void CCombinedArtifactInstance::addAsConstituent(CArtifactInstance *art, ArtifactPosition slot)
{
	assert(vstd::contains(*artType->constituents, art->artType.get()));
	assert(art->getParentNodes().size() == 1  &&  art->getParentNodes().front() == art->artType);
	constituentsInfo.push_back(ConstituentInfo(art, slot));
	attachTo(*art);
}

void CCombinedArtifactInstance::putAt(ArtifactLocation al)
{
	if(al.slot >= GameConstants::BACKPACK_START)
	{
		CArtifactInstance::putAt(al);
		for(ConstituentInfo &ci : constituentsInfo)
			ci.slot = ArtifactPosition::PRE_FIRST;
	}
	else
	{
		CArtifactInstance *mainConstituent = figureMainConstituent(al); //it'll be replaced with combined artifact, not a lock
		CArtifactInstance::putAt(al); //puts combined art (this)

		for(ConstituentInfo &ci : constituentsInfo)
		{
			if(ci.art != mainConstituent)
			{
				const ArtifactLocation suggestedPos(al.artHolder, ci.slot);
				const bool inActiveSlot = vstd::isbetween(ci.slot, 0, GameConstants::BACKPACK_START);
				const bool suggestedPosValid = ci.art->canBePutAt(suggestedPos);

				ArtifactPosition pos = ArtifactPosition::PRE_FIRST;
				if(inActiveSlot  &&  suggestedPosValid) //there is a valid suggestion where to place lock
					pos = ci.slot;
				else
					ci.slot = pos = ci.art->firstAvailableSlot(al.getHolderArtSet());

				assert(pos < GameConstants::BACKPACK_START);
				al.getHolderArtSet()->setNewArtSlot(pos, ci.art, true); //sets as lock
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
	if(al.slot >= GameConstants::BACKPACK_START)
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

CArtifactInstance * CCombinedArtifactInstance::figureMainConstituent(const ArtifactLocation al)
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

CCombinedArtifactInstance::ConstituentInfo::ConstituentInfo(CArtifactInstance *Art, ArtifactPosition Slot)
{
	art = Art;
	slot = Slot;
}

bool CCombinedArtifactInstance::ConstituentInfo::operator==(const ConstituentInfo &rhs) const
{
	return art == rhs.art && slot == rhs.slot;
}

CArtifactSet::~CArtifactSet() = default;

const CArtifactInstance* CArtifactSet::getArt(ArtifactPosition pos, bool excludeLocked) const
{
	if(const ArtSlotInfo *si = getSlot(pos))
	{
		if(si->artifact && (!excludeLocked || !si->locked))
			return si->artifact;
	}

	return nullptr;
}

CArtifactInstance* CArtifactSet::getArt(ArtifactPosition pos, bool excludeLocked)
{
	return const_cast<CArtifactInstance*>((const_cast<const CArtifactSet*>(this))->getArt(pos, excludeLocked));
}

ArtifactPosition CArtifactSet::getArtPos(int aid, bool onlyWorn, bool allowLocked) const
{
	const auto result = getAllArtPositions(aid, onlyWorn, allowLocked, false);
	return result.empty() ? ArtifactPosition{ArtifactPosition::PRE_FIRST} : result[0];
}

ArtifactPosition CArtifactSet::getArtBackpackPos(int aid) const
{
	const auto result = getBackpackArtPositions(aid);
	return result.empty() ? ArtifactPosition{ArtifactPosition::PRE_FIRST} : result[0];
}

std::vector<ArtifactPosition> CArtifactSet::getAllArtPositions(int aid, bool onlyWorn, bool allowLocked, bool getAll) const
{
	std::vector<ArtifactPosition> result;
	for(auto & slotInfo : artifactsWorn)
		if(slotInfo.second.artifact->artType->id == aid && (allowLocked || !slotInfo.second.locked))
			result.push_back(slotInfo.first);

	if(onlyWorn)
		return result;
	if(!getAll && !result.empty())
		return result;

	auto backpackPositions = getBackpackArtPositions(aid);
	result.insert(result.end(), backpackPositions.begin(), backpackPositions.end());
	return result;
}

std::vector<ArtifactPosition> CArtifactSet::getBackpackArtPositions(int aid) const
{
	std::vector<ArtifactPosition> result;

	si32 backpackPosition = GameConstants::BACKPACK_START;
	for(auto & artInfo : artifactsInBackpack)
	{
		auto * art = artInfo.getArt();
		if(art && art->artType->id == aid)
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

const CArtifactInstance * CArtifactSet::getArtByInstanceId( ArtifactInstanceID artInstId ) const
{
	for(auto i : artifactsWorn)
		if(i.second.artifact->id == artInstId)
			return i.second.artifact;

	for(auto i : artifactsInBackpack)
		if(i.artifact->id == artInstId)
			return i.artifact;

	return nullptr;
}

bool CArtifactSet::hasArt(
	ui32 aid,
	bool onlyWorn,
    bool searchBackpackAssemblies,
	bool allowLocked) const
{
	return getArtPosCount(aid, onlyWorn, searchBackpackAssemblies, allowLocked) > 0;
}

bool CArtifactSet::hasArtBackpack(ui32 aid) const
{
	return getBackpackArtPositions(aid).size() > 0;
}

unsigned CArtifactSet::getArtPosCount(int aid, bool onlyWorn, bool searchBackpackAssemblies, bool allowLocked) const
{
	const auto allPositions = getAllArtPositions(aid, onlyWorn, allowLocked, true);
	if(!allPositions.empty())
		return allPositions.size();

	if(searchBackpackAssemblies && getHiddenArt(aid))
		return 1;

	return 0;
}

std::pair<const CCombinedArtifactInstance *, const CArtifactInstance *>
CArtifactSet::searchForConstituent(int aid) const
{
	for(auto & slot : artifactsInBackpack)
	{
		auto art = slot.artifact;
		if(art->canBeDisassembled())
		{
			auto ass = static_cast<CCombinedArtifactInstance *>(art.get());
			for(auto& ci : ass->constituentsInfo)
			{
				if(ci.art->artType->id == aid)
				{
					return {ass, ci.art};
				}
			}
		}
	}
	return {nullptr, nullptr};
}

const CArtifactInstance *CArtifactSet::getHiddenArt(int aid) const
{
	return searchForConstituent(aid).second;
}

const CCombinedArtifactInstance *CArtifactSet::getAssemblyByConstituent(int aid) const
{
	return searchForConstituent(aid).first;
}

const ArtSlotInfo * CArtifactSet::getSlot(ArtifactPosition pos) const
{
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

bool CArtifactSet::isPositionFree(ArtifactPosition pos, bool onlyLockCheck) const
{
	if(const ArtSlotInfo *s = getSlot(pos))
		return (onlyLockCheck || !s->artifact) && !s->locked;

	return true; //no slot means not used
}

ArtSlotInfo & CArtifactSet::retrieveNewArtSlot(ArtifactPosition slot)
{
	assert(!vstd::contains(artifactsWorn, slot));
	ArtSlotInfo &ret = slot < GameConstants::BACKPACK_START
		? artifactsWorn[slot]
		: *artifactsInBackpack.insert(artifactsInBackpack.begin() + (slot - GameConstants::BACKPACK_START), ArtSlotInfo());

	return ret;
}

void CArtifactSet::setNewArtSlot(ArtifactPosition slot, CArtifactInstance *art, bool locked)
{
	ArtSlotInfo & asi = retrieveNewArtSlot(slot);
	asi.artifact = art;
	asi.locked = locked;
}

void CArtifactSet::eraseArtSlot(ArtifactPosition slot)
{
	if(slot < GameConstants::BACKPACK_START)
	{
		artifactsWorn.erase(slot);
	}
	else
	{
		slot = ArtifactPosition(slot - GameConstants::BACKPACK_START);
		artifactsInBackpack.erase(artifactsInBackpack.begin() + slot);
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
			backpackTemp.push_back(info.artifact->artType->id);
	}
	handler.serializeIdArray(NArtifactPosition::backpack, backpackTemp);
	if(!handler.saving)
	{
        for(const ArtifactID & artifactID : backpackTemp)
		{
			auto artifact = CArtifactInstance::createArtifact(map, artifactID.toEnum());
			auto slot = ArtifactPosition(GameConstants::BACKPACK_START + (si32)artifactsInBackpack.size());
			if(artifact->canBePutAt(this, slot))
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
			artifactID = info->artifact->artType->id;
			handler.serializeId(NArtifactPosition::namesHero[slot.num], artifactID, ArtifactID::NONE);
		}
	}
	else
	{
		handler.serializeId(NArtifactPosition::namesHero[slot.num], artifactID, ArtifactID::NONE);

		if(artifactID != ArtifactID::NONE)
		{
			auto artifact = CArtifactInstance::createArtifact(map, artifactID.toEnum());

			if(artifact->canBePutAt(this, slot))
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

CArtifactFittingSet::CArtifactFittingSet(ArtBearer::ArtBearer Bearer)
{
	this->Bearer = Bearer;
}

void CArtifactFittingSet::setNewArtSlot(ArtifactPosition slot, CArtifactInstance * art, bool locked)
{
	ArtSlotInfo & asi = retrieveNewArtSlot(slot);
	asi.artifact = art;
	asi.locked = locked;
}

void CArtifactFittingSet::putArtifact(ArtifactPosition pos, CArtifactInstance * art)
{
	if(art && art->canBeDisassembled() && (pos < ArtifactPosition::AFTER_LAST))
	{
		for(auto & part : dynamic_cast<CCombinedArtifactInstance*>(art)->constituentsInfo)
		{
			// For the ArtFittingSet is no needed to do figureMainConstituent, just lock slots
			this->setNewArtSlot(part.art->firstAvailableSlot(this), part.art, true);
		}
	}
	else
	{
		this->setNewArtSlot(pos, art, false);
	}
}

ArtBearer::ArtBearer CArtifactFittingSet::bearerType() const
{
	return this->Bearer;
}

DLL_LINKAGE ArtifactPosition ArtifactUtils::getArtifactDstPosition(	const CArtifactInstance * artifact,
									const CArtifactSet * target, 
									ArtBearer::ArtBearer bearer)
{
	for(auto slot : artifact->artType->possibleSlots.at(bearer))
	{
		auto existingArtifact = target->getArt(slot);
		auto existingArtInfo = target->getSlot(slot);

		if(!existingArtifact
			&& (!existingArtInfo || !existingArtInfo->locked)
			&& artifact->canBePutAt(target, slot))
		{
			return slot;
		}
	}
	return ArtifactPosition(GameConstants::BACKPACK_START);
}

DLL_LINKAGE std::vector<ArtifactPosition> ArtifactUtils::unmovablePositions()
{
	return { ArtifactPosition::SPELLBOOK, ArtifactPosition::MACH4 };
}

DLL_LINKAGE bool ArtifactUtils::isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot)
{
	return slot.second.artifact
		&& !slot.second.locked
		&& !vstd::contains(unmovablePositions(), slot.first);
}

DLL_LINKAGE bool ArtifactUtils::checkSpellbookIsNeeded(const CGHeroInstance * heroPtr, ArtifactID artID, ArtifactPosition slot)
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

DLL_LINKAGE bool ArtifactUtils::isSlotBackpack(ArtifactPosition slot)
{
	return slot >= GameConstants::BACKPACK_START;
}

VCMI_LIB_NAMESPACE_END
