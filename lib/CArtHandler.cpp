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
#include "spells/CSpellHandler.h"
#include "mapObjects/MapObjects.h"
#include "NetPacksBase.h"
#include "GameConstants.h"
#include "CRandomGenerator.h"

#include "mapObjects/CObjectClassesHandler.h"

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

const std::string & CArtifact::Name() const
{
	return name;
}

const std::string & CArtifact::Description() const
{
	return description;
}
const std::string & CArtifact::EventText() const
{
	return eventText;
}

bool CArtifact::isBig () const
{
	return VLC->arth->isBigArtifact(id);
}

bool CArtifact::isTradable () const
{
	return VLC->arth->isTradableArtifact(id);
}

CArtifact::CArtifact()
{
	setNodeType(ARTIFACT);
	possibleSlots[ArtBearer::HERO]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::CREATURE]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::COMMANDER];
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
	return "Artifact: " + Name();
}

void CArtifact::addNewBonus(Bonus *b)
{
	b->source = Bonus::ARTIFACT;
	b->duration = Bonus::PERMANENT;
	b->description = name;
	CBonusSystemNode::addNewBonus(b);
}

void CGrowingArtifact::levelUpArtifact (CArtifactInstance * art)
{
	Bonus b;
	b.type = Bonus::LEVEL_COUNTER;
	b.val = 1;
	b.duration = Bonus::COMMANDER_KILLED;
	art->accumulateBonus (b);

	for (auto bonus : bonusesPerLevel)
	{
		if (art->valOfBonuses(Bonus::LEVEL_COUNTER) % bonus.first == 0) //every n levels
		{
			art->accumulateBonus (bonus.second);
		}
	}
	for (auto bonus : thresholdBonuses)
	{
		if (art->valOfBonuses(Bonus::LEVEL_COUNTER) == bonus.first) //every n levels
		{
			art->addNewBonus (&bonus.second);
		}
	}
}

CArtHandler::CArtHandler()
{
	//VLC->arth = this;

	// War machines are the default big artifacts.
	for (ArtifactID i = ArtifactID::CATAPULT; i <= ArtifactID::FIRST_AID_TENT; i.advance(1))
		bigArtifacts.insert(i);
}

CArtHandler::~CArtHandler()
{
	for(CArtifact * art : artifacts)
		delete art;
}

std::vector<JsonNode> CArtHandler::loadLegacyData(size_t dataSize)
{
	artifacts.resize(dataSize);
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
	auto object = loadFromJson(data);
	object->id = ArtifactID(artifacts.size());
	object->iconIndex = object->id + 5;

	artifacts.push_back(object);

	VLC->modh->identifiers.registerObject(scope, "artifact", name, object->id);
}

void CArtHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(data);
	object->id = ArtifactID(index);
	object->iconIndex = object->id;

	assert(artifacts[index] == nullptr); // ensure that this id was not loaded before
	artifacts[index] = object;

	VLC->modh->identifiers.registerObject(scope, "artifact", name, object->id);
}

CArtifact * CArtHandler::loadFromJson(const JsonNode & node)
{
	CArtifact * art;

	if (!VLC->modh->modules.COMMANDERS || node["growing"].isNull())
		art = new CArtifact();
	else
	{
		auto  growing = new CGrowingArtifact();
		loadGrowingArt(growing, node);
		art = growing;
	}

	const JsonNode & text = node["text"];
	art->name        = text["name"].String();
	art->description = text["description"].String();
	art->eventText   = text["event"].String();

	const JsonNode & graphics = node["graphics"];
	art->image = graphics["image"].String();

	if (!graphics["large"].isNull())
		art->large = graphics["large"].String();
	else
		art->large = art->image;

	art->advMapDef = graphics["map"].String();

	art->price = node["value"].Float();

	loadSlots(art, node);
	loadClass(art, node);
	loadType(art, node);
	loadComponents(art, node);

	for (auto b : node["bonuses"].Vector())
	{
		auto bonus = JsonUtils::parseBonus (b);
		art->addNewBonus(bonus);
	}
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

	logGlobal->warnStream() << "Warning! Artifact slot " << slotName << " not recognized!";
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
		if (node["slot"].getType() == JsonNode::DATA_STRING)
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

	logGlobal->warnStream() << "Warning! Artifact rarity " << className << " not recognized!";
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
            logGlobal->warnStream() << "Warning! Artifact type " << b.String() << " not recognized!";
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
				art->constituents->push_back(VLC->arth->artifacts[id]);
				VLC->arth->artifacts[id]->constituentOf.push_back(art);
			});
		}
	}
}

void CArtHandler::loadGrowingArt(CGrowingArtifact * art, const JsonNode & node)
{
	for (auto b : node["growing"]["bonusesPerLevel"].Vector())
	{
		art->bonusesPerLevel.push_back (std::pair <ui16, Bonus> (b["level"].Float(), *JsonUtils::parseBonus (b["bonus"])));
	}
	for (auto b : node["growing"]["thresholdBonuses"].Vector())
	{
		art->thresholdBonuses.push_back (std::pair <ui16, Bonus> (b["level"].Float(), *JsonUtils::parseBonus (b["bonus"])));
	}
}

//TODO: use bimap
ArtifactID CArtHandler::creatureToMachineID(CreatureID id)
{
	switch (id)
	{
	case CreatureID::CATAPULT: //Catapult
		return ArtifactID::CATAPULT;
		break;
	case CreatureID::BALLISTA: //Ballista
		return ArtifactID::BALLISTA;
		break;
	case CreatureID::FIRST_AID_TENT: //First Aid tent
		return ArtifactID::FIRST_AID_TENT;
		break;
	case CreatureID::AMMO_CART: //Ammo cart
		return ArtifactID::AMMO_CART;
		break;
	}
	return ArtifactID::NONE; //this creature is not artifact
}

CreatureID CArtHandler::machineIDToCreature(ArtifactID id)
{
	switch (id)
	{
	case ArtifactID::CATAPULT:
		return CreatureID::CATAPULT;
		break;
	case ArtifactID::BALLISTA:
		return CreatureID::BALLISTA;
		break;
	case ArtifactID::FIRST_AID_TENT:
		return CreatureID::FIRST_AID_TENT;
		break;
	case ArtifactID::AMMO_CART:
		return CreatureID::AMMO_CART;
		break;
	}
	return CreatureID::NONE; //this artifact is not a creature
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
			std::fill_n (out.begin(), 64, artifacts[2]); //Give Grail - this can't be banned (hopefully)
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

Bonus *createBonus(Bonus::BonusType type, int val, int subtype, Bonus::ValueType valType, std::shared_ptr<ILimiter> limiter = std::shared_ptr<ILimiter>(), int additionalInfo = 0)
{
	auto added = new Bonus(Bonus::PERMANENT,type,Bonus::ARTIFACT,val,-1,subtype);
	added->additionalInfo = additionalInfo;
	added->valType = valType;
	added->limiter = limiter;
	return added;
}

Bonus *createBonus(Bonus::BonusType type, int val, int subtype, std::shared_ptr<IPropagator> propagator = std::shared_ptr<IPropagator>(), int additionalInfo = 0)
{
	auto added = new Bonus(Bonus::PERMANENT,type,Bonus::ARTIFACT,val,-1,subtype);
	added->additionalInfo = additionalInfo;
	added->valType = Bonus::BASE_NUMBER;
	added->propagator = propagator;
	return added;
}

void CArtHandler::giveArtBonus( ArtifactID aid, Bonus::BonusType type, int val, int subtype, Bonus::ValueType valType, std::shared_ptr<ILimiter> limiter, int additionalInfo)
{
	giveArtBonus(aid, createBonus(type, val, subtype, valType, limiter, additionalInfo));
}

void CArtHandler::giveArtBonus(ArtifactID aid, Bonus::BonusType type, int val, int subtype, std::shared_ptr<IPropagator> propagator /*= nullptr*/, int additionalInfo)
{
	giveArtBonus(aid, createBonus(type, val, subtype, propagator, additionalInfo));
}

void CArtHandler::giveArtBonus(ArtifactID aid, Bonus *bonus)
{
	bonus->sid = aid;
	if(bonus->subtype == Bonus::MORALE || bonus->type == Bonus::LUCK)
		bonus->description = artifacts[aid]->Name()  + (bonus->val > 0 ? " +" : " ") + boost::lexical_cast<std::string>(bonus->val);
	else
		bonus->description = artifacts[aid]->Name();

	artifacts[aid]->addNewBonus(bonus);
}
void CArtHandler::makeItCreatureArt (CArtifact * a, bool onlyCreature /*=true*/)
{
	if (onlyCreature)
	{
		a->possibleSlots[ArtBearer::HERO].clear();
		a->possibleSlots[ArtBearer::COMMANDER].clear();
	}
	a->possibleSlots[ArtBearer::CREATURE].push_back(ArtifactPosition::CREATURE_SLOT);
}

void CArtHandler::makeItCreatureArt (ArtifactID aid, bool onlyCreature /*=true*/)
{
	CArtifact *a = artifacts[aid];
	makeItCreatureArt (a, onlyCreature);
}

void CArtHandler::makeItCommanderArt (CArtifact * a, bool onlyCommander /*= true*/ )
{
	if (onlyCommander)
	{
		a->possibleSlots[ArtBearer::HERO].clear();
		a->possibleSlots[ArtBearer::CREATURE].clear();
	}
	for (int i = ArtifactPosition::COMMANDER1; i <= ArtifactPosition::COMMANDER6; ++i)
		a->possibleSlots[ArtBearer::COMMANDER].push_back(ArtifactPosition(i));
}

void CArtHandler::makeItCommanderArt( ArtifactID aid, bool onlyCommander /*= true*/ )
{
	CArtifact *a = artifacts[aid];
	makeItCommanderArt (a, onlyCommander);
}

bool CArtHandler::legalArtifact(ArtifactID id)
{
	auto art = artifacts[id];
	//assert ( (!art->constituents) || art->constituents->size() ); //artifacts is not combined or has some components
	return ((art->possibleSlots[ArtBearer::HERO].size() ||
		(art->possibleSlots[ArtBearer::COMMANDER].size() && VLC->modh->modules.COMMANDERS) ||
		(art->possibleSlots[ArtBearer::CREATURE].size() && VLC->modh->modules.STACK_ARTIFACT)) &&
		!(art->constituents) && //no combo artifacts spawning
		art->aClass >= CArtifact::ART_TREASURE &&
		art->aClass <= CArtifact::ART_RELIC);
}

bool CArtHandler::isTradableArtifact(ArtifactID id) const
{
	switch (id)
	{
	case ArtifactID::SPELLBOOK:
	case ArtifactID::GRAIL:
	case ArtifactID::CATAPULT:
	case ArtifactID::BALLISTA:
	case ArtifactID::AMMO_CART:
	case ArtifactID::FIRST_AID_TENT:
		return false;
	default:
		return true;
	}
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
			allowedArtifacts.push_back(artifacts[i]);
	}
	for (ArtifactID i = ArtifactID::ART_SELECTION; i<ArtifactID(artifacts.size()); i.advance(1)) //try to allow all artifacts added by mods
	{
		if (legalArtifact(ArtifactID(i)))
			allowedArtifacts.push_back(artifacts[i]);
			 //keep im mind that artifact can be worn by more than one type of bearer
	}
}

std::vector<bool> CArtHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedArtifacts;
	allowedArtifacts.resize(127, true);
	allowedArtifacts.resize(141, false);
	allowedArtifacts.resize(GameConstants::ARTIFACTS_QUANTITY, true);
	return allowedArtifacts;
}

void CArtHandler::erasePickedArt(ArtifactID id)
{
	CArtifact *art = artifacts[id];

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
            logGlobal->warnStream() << "Problem: cannot erase artifact " << art->Name() << " from list, it was not present";

	}
	else
        logGlobal->warnStream() << "Problem: cannot find list for artifact " << art->Name() << ", strange class. (special?)";
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
	for(auto &art : artifacts)
	{
		for(auto &bonus : art->getExportedBonusList())
		{
			assert(art == artifacts[art->id]);
			assert(bonus->source == Bonus::ARTIFACT);
			bonus->sid = art->id;
		}
	}
	CBonusSystemNode::treeHasChanged();

	for (CArtifact * art : artifacts)
	{
		VLC->objtypeh->loadSubObject(art->Name(), JsonNode(), Obj::ARTIFACT, art->id.num);

		if (!art->advMapDef.empty())
		{
			JsonNode templ;
			templ["animation"].String() = art->advMapDef;

			// add new template.
			// Necessary for objects added via mods that don't have any templates in H3
			VLC->objtypeh->getHandlerFor(Obj::ARTIFACT, art->id)->addTemplate(templ);
		}
		// object does not have any templates - this is not usable object (e.g. pseudo-art like lock)
		if (VLC->objtypeh->getHandlerFor(Obj::ARTIFACT, art->id)->getTemplates().empty())
			VLC->objtypeh->removeSubObject(Obj::ARTIFACT, art->id);
	}
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
	attachTo(Art);
}

std::string CArtifactInstance::nodeName() const
{
	return "Artifact instance of " + (artType ? artType->Name() : std::string("uninitialized")) + " type";
}

CArtifactInstance * CArtifactInstance::createScroll( const CSpell *s)
{
	return createScroll(s->id);
}

CArtifactInstance *CArtifactInstance::createScroll(SpellID sid)
{
	auto ret = new CArtifactInstance(VLC->arth->artifacts[ArtifactID::SPELL_SCROLL]);
	auto b = new Bonus(Bonus::PERMANENT, Bonus::SPELL, Bonus::ARTIFACT_INSTANCE, -1, ArtifactID::SPELL_SCROLL, sid);
	ret->addNewBonus(b);
	return ret;
}

void CArtifactInstance::init()
{
	id = ArtifactInstanceID();
	id = static_cast<ArtifactInstanceID>(ArtifactID::NONE); //to be randomized
	setNodeType(ARTIFACT_INSTANCE);
}

std::string CArtifactInstance::getEffectiveDescription(
	const CGHeroInstance *hero) const
{
	std::string text = artType->Description();
	if (!vstd::contains(text, '{'))
		text = '{' + artType->Name() + "}\n\n" + text; //workaround for new artifacts with single name, turns it to H3-style

	if(artType->id == ArtifactID::SPELL_SCROLL)
	{
		// we expect scroll description to be like this: This scroll contains the [spell name] spell which is added into your spell book for as long as you carry the scroll.
		// so we want to replace text in [...] with a spell name
		// however other language versions don't have name placeholder at all, so we have to be careful
		int spellID = getGivenSpellID();
		size_t nameStart = text.find_first_of('[');
		size_t nameEnd = text.find_first_of(']', nameStart);
		if(spellID >= 0)
		{
			if(nameStart != std::string::npos  &&  nameEnd != std::string::npos)
				text = text.replace(nameStart, nameEnd - nameStart + 1, VLC->spellh->objects[spellID]->name);
		}
	}
	else if (hero && artType->constituentOf.size()) //display info about set
	{
		std::string artList;
		auto combinedArt = artType->constituentOf[0];
		text += "\n\n";
		text += "{" + combinedArt->Name() + "}";
		int wornArtifacts = 0;
		for (auto a : *combinedArt->constituents) //TODO: can the artifact be a part of more than one set?
		{
			artList += "\n" + a->Name();
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
			GameConstants::BACKPACK_START + h->artifactsInBackpack.size());

	return ArtifactPosition::PRE_FIRST;
}

bool CArtifactInstance::canBePutAt(const ArtifactLocation & al, bool assumeDestRemoved /*= false*/) const
{
	return canBePutAt(al.getHolderArtSet(), al.slot, assumeDestRemoved);
}

bool CArtifactInstance::canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved /*= false*/) const
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
        logGlobal->warnStream() << "Warning: artifact " << artType->Name() << " doesn't have defined allowed slots for bearer of type "
            << artSet->bearerType();
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
		al.getHolderNode()->attachTo(this);
}

void CArtifactInstance::removeFrom(ArtifactLocation al)
{
	assert(al.getHolderArtSet()->getArt(al.slot) == this);
	al.getHolderArtSet()->eraseArtSlot(al.slot);
	if(al.slot < GameConstants::BACKPACK_START)
		al.getHolderNode()->detachFrom(this);

	//TODO delete me?
}

bool CArtifactInstance::canBeDisassembled() const
{
	return bool(artType->constituents);
}

std::vector<const CArtifact *> CArtifactInstance::assemblyPossibilities(const CArtifactSet *h) const
{
	std::vector<const CArtifact *> ret;
	if(artType->constituents) //combined artifact already: no combining of combined artifacts... for now.
		return ret;

	for(const CArtifact * artifact : artType->constituentOf)
	{
		assert(artifact->constituents);
		bool possible = true;

		for(const CArtifact * constituent : *artifact->constituents) //check if all constituents are available
		{
			if(!h->hasArt(constituent->id, true)) //constituent must be equipped
			{
				possible = false;
				break;
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
			auto  bonus = new Bonus;
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
	return createNewArtifactInstance(VLC->arth->artifacts[aid]);
}

void CArtifactInstance::deserializationFix()
{
	setType(artType);
}

SpellID CArtifactInstance::getGivenSpellID() const
{
	const Bonus * b = getBonusLocalFirst(Selector::type(Bonus::SPELL));
	if(!b)
	{
		logGlobal->warnStream() << "Warning: " << nodeName() << " doesn't bear any spell!";
		return SpellID::NONE;
	}
	return SpellID(b->subtype);
}

bool CArtifactInstance::isPart(const CArtifactInstance *supposedPart) const
{
	return supposedPart == this;
}

bool CCombinedArtifactInstance::canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved /*= false*/) const
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
	: CArtifactInstance(Art) //TODO: seems unued, but need to be written
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
	attachTo(art);
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
		attachTo(ci.art);
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

CCombinedArtifactInstance::ConstituentInfo::ConstituentInfo(CArtifactInstance *Art /*= nullptr*/, ArtifactPosition Slot /*= -1*/)
{
	art = Art;
	slot = Slot;
}

bool CCombinedArtifactInstance::ConstituentInfo::operator==(const ConstituentInfo &rhs) const
{
	return art == rhs.art && slot == rhs.slot;
}

const CArtifactInstance* CArtifactSet::getArt(ArtifactPosition pos, bool excludeLocked /*= true*/) const
{
	if(const ArtSlotInfo *si = getSlot(pos))
	{
		if(si->artifact && (!excludeLocked || !si->locked))
			return si->artifact;
	}

	return nullptr;
}

CArtifactInstance* CArtifactSet::getArt(ArtifactPosition pos, bool excludeLocked /*= true*/)
{
	return const_cast<CArtifactInstance*>((const_cast<const CArtifactSet*>(this))->getArt(pos, excludeLocked));
}

ArtifactPosition CArtifactSet::getArtPos(int aid, bool onlyWorn /*= true*/) const
{
	for(auto i = artifactsWorn.cbegin(); i != artifactsWorn.cend(); i++)
		if(i->second.artifact->artType->id == aid)
			return i->first;

	if(onlyWorn)
		return ArtifactPosition::PRE_FIRST;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact->artType->id == aid)
			return ArtifactPosition(GameConstants::BACKPACK_START + i);

	return ArtifactPosition::PRE_FIRST;
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

bool CArtifactSet::hasArt(ui32 aid, bool onlyWorn /*= false*/,
                          bool searchBackpackAssemblies /*= false*/) const
{
	return getArtPos(aid, onlyWorn) != ArtifactPosition::PRE_FIRST ||
	       (searchBackpackAssemblies && getHiddenArt(aid));
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

bool CArtifactSet::isPositionFree(ArtifactPosition pos, bool onlyLockCheck /*= false*/) const
{
	if(const ArtSlotInfo *s = getSlot(pos))
		return (onlyLockCheck || !s->artifact) && !s->locked;

	return true; //no slot means not used
}

si32 CArtifactSet::getArtTypeId(ArtifactPosition pos) const
{
	const CArtifactInstance * const a = getArt(pos);
	if(!a)
	{
        logGlobal->warnStream() << (dynamic_cast<const CGHeroInstance*>(this))->name << " has no artifact at " << pos << " (getArtTypeId)";
		return -1;
	}
	return a->artType->id;
}

CArtifactSet::~CArtifactSet()
{

}

ArtSlotInfo & CArtifactSet::retreiveNewArtSlot(ArtifactPosition slot)
{
	assert(!vstd::contains(artifactsWorn, slot));
	ArtSlotInfo &ret = slot < GameConstants::BACKPACK_START
		? artifactsWorn[slot]
		: *artifactsInBackpack.insert(artifactsInBackpack.begin() + (slot - GameConstants::BACKPACK_START), ArtSlotInfo());

	return ret;
}

void CArtifactSet::setNewArtSlot(ArtifactPosition slot, CArtifactInstance *art, bool locked)
{
	ArtSlotInfo &asi = retreiveNewArtSlot(slot);
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
			node->attachTo(elem.second.artifact);
}
