#include "StdInc.h"
#include "CArtHandler.h"

#include "filesystem/CResourceLoader.h"
#include "CGeneralTextHandler.h"
#include <boost/random/linear_congruential.hpp>
#include "VCMI_Lib.h"
#include "CModHandler.h"
#include "CSpellHandler.h"
#include "CObjectHandler.h"
#include "NetPacks.h"
#include "GameConstants.h"

using namespace boost::assign;

/*
 * CArtHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern boost::rand48 ran;
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
	ART_POS(HEAD);

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
	if(id == 1)
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

	BOOST_FOREACH (auto bonus, bonusesPerLevel)
	{
		if (art->valOfBonuses(Bonus::LEVEL_COUNTER) % bonus.first == 0) //every n levels
		{
			art->accumulateBonus (bonus.second);
		}
	}
	BOOST_FOREACH (auto bonus, thresholdBonuses)
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
}

void CArtHandler::load(bool onlyTxt)
{
	if (onlyTxt)
		return; // looks to be broken anyway...

	#define ART_POS(x) (  #x)
	const std::vector<std::string> artSlots = boost::assign::list_of ART_POS_LIST;
	#undef ART_POS

	static std::map<char, std::string> classes =
	  map_list_of('S',"SPECIAL")('T',"TREASURE")('N',"MINOR")('J',"MAJOR")('R',"RELIC");

	CLegacyConfigParser parser("DATA/ARTRAITS.TXT");
	CLegacyConfigParser events("DATA/ARTEVENT.TXT");

	parser.endLine(); // header
	parser.endLine();

	std::map<ui32,ui8>::iterator itr;

	std::vector<JsonNode> h3Data;

	for (size_t i = 0; i < GameConstants::ARTIFACTS_QUANTITY; i++)
	{
		JsonNode artData;

		artData["graphics"]["iconIndex"].Float() = i;
		artData["text"]["name"].String() = parser.readString();
		artData["text"]["event"].String() = events.readString();
		artData["value"].Float() = parser.readNumber();

		for(int j=0; j<artSlots.size(); j++)
		{
			if(parser.readString() == "x")
			{
				artData["slot"].Vector().push_back(JsonNode());
				artData["slot"].Vector().back().String() = artSlots.at(j);
			}
		}
		artData["class"].String() = classes[parser.readString()[0]];
		artData["text"]["description"].String() = parser.readString();

		parser.endLine();
		events.endLine();
		h3Data.push_back(artData);
	}

	artifacts.resize(GameConstants::ARTIFACTS_QUANTITY);

	JsonNode config(ResourceID("config/artifacts.json"));

	BOOST_FOREACH(auto & node, config["artifacts"].Struct())
	{
		int numeric = node.second["id"].Float();
		JsonNode & artData = h3Data[numeric];
		JsonUtils::merge(artData, node.second);

		artifacts[numeric] = loadArtifact(artData);
		artifacts[numeric]->id = ArtifactID(numeric);

		VLC->modh->identifiers.registerObject ("artifact." + node.first, numeric);
	}

	for (size_t i=0; i < artifacts.size(); i++)
	{
		if (artifacts[i] == nullptr)
			tlog0 << "Warning: artifact with id " << i << " is missing!\n";
	}
}

void CArtHandler::load(std::string objectID, const JsonNode & node)
{
	CArtifact * art = loadArtifact(node);
	art->id = ArtifactID(artifacts.size());

	artifacts.push_back(art);
	tlog5 << "Added artifact: " << objectID << "\n";
	VLC->modh->identifiers.registerObject ("artifact." + objectID, art->id);
}

CArtifact * CArtHandler::loadArtifact(const JsonNode & node)
{
	CArtifact * art;

	if (!VLC->modh->modules.COMMANDERS || node["growing"].isNull())
		art = new CArtifact();
	else
	{
		CGrowingArtifact * growing = new CGrowingArtifact();
		loadGrowingArt(growing, node);
		art = growing;
	}

	const JsonNode & text = node["text"];
	art->name        = text["name"].String();
	art->description = text["description"].String();
	art->eventText   = text["event"].String();

	const JsonNode & graphics = node["graphics"];
	art->iconIndex = graphics["iconIndex"].Float();
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

	BOOST_FOREACH (auto b, node["bonuses"].Vector())
	{
		auto bonus = JsonUtils::parseBonus (b);
		bonus->sid = art->id;
		art->addNewBonus (bonus);
	}
	return art;
}

void CArtHandler::addSlot(CArtifact * art, const std::string & slotID)
{
#define ART_POS(x) ( #x, ArtifactPosition::x )
	static const std::map<std::string, ArtifactPosition> artifactPositionMap = boost::assign::map_list_of ART_POS_LIST;
#undef ART_POS

	if (slotID == "MISC")
	{
		art->possibleSlots[ArtBearer::HERO] += ArtifactPosition::MISC1, ArtifactPosition::MISC2, ArtifactPosition::MISC3, ArtifactPosition::MISC4, ArtifactPosition::MISC5;
	}
	else if (slotID == "RING")
	{
		art->possibleSlots[ArtBearer::HERO] += ArtifactPosition::LEFT_RING, ArtifactPosition::RIGHT_RING;
	}
	else
	{
		auto it = artifactPositionMap.find (slotID);
		if (it != artifactPositionMap.end())
		{
			auto slot = it->second;
			art->possibleSlots[ArtBearer::HERO].push_back (slot);
		}
		else
			tlog2 << "Warning! Artifact slot " << slotID << " not recognized!";
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
			BOOST_FOREACH (const JsonNode & slot, node["slot"].Vector())
				addSlot(art, slot.String());
		}
	}
}

void CArtHandler::loadClass(CArtifact * art, const JsonNode & node)
{
	static const std::map<std::string, CArtifact::EartClass> artifactClassMap = boost::assign::map_list_of
		("TREASURE", CArtifact::ART_TREASURE)
		("MINOR", CArtifact::ART_MINOR)
		("MAJOR", CArtifact::ART_MAJOR)
		("RELIC", CArtifact::ART_RELIC)
		("SPECIAL", CArtifact::ART_SPECIAL);

	auto it = artifactClassMap.find (node["class"].String());
	if (it != artifactClassMap.end())
	{
		art->aClass = it->second;
	}
	else
	{
		tlog2 << "Warning! Artifact rarity " << node["class"].String() << " not recognized!";
		art->aClass = CArtifact::ART_SPECIAL;
	}
}

void CArtHandler::loadType(CArtifact * art, const JsonNode & node)
{
#define ART_BEARER(x) ( #x, ArtBearer::x )
	static const std::map<std::string, int> artifactBearerMap = boost::assign::map_list_of ART_BEARER_LIST;
#undef ART_BEARER

	BOOST_FOREACH (const JsonNode & b, node["type"].Vector())
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
			tlog2 << "Warning! Artifact type " << b.String() << " not recognized!";
	}
}

void CArtHandler::loadComponents(CArtifact * art, const JsonNode & node)
{
	if (!node["components"].isNull())
	{
		art->constituents.reset(new std::vector<CArtifact *>());
		BOOST_FOREACH (auto component, node["components"].Vector())
		{
			VLC->modh->identifiers.requestIdentifier("artifact." + component.String(), [=](si32 id)
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
	BOOST_FOREACH (auto b, node["growing"]["bonusesPerLevel"].Vector())
	{
		art->bonusesPerLevel.push_back (std::pair <ui16, Bonus> (b["level"].Float(), *JsonUtils::parseBonus (b["bonus"].Vector())));
	}
	BOOST_FOREACH (auto b, node["growing"]["thresholdBonuses"].Vector())
	{
		art->thresholdBonuses.push_back (std::pair <ui16, Bonus> (b["level"].Float(), *JsonUtils::parseBonus (b["bonus"].Vector())));
	}
}

ArtifactID CArtHandler::creatureToMachineID(CreatureID id)
{
	int dif = 142;
	switch (id)
	{
	case 147:
		dif--;
		break;
	case 148:
		dif++;
		break;
	}
	dif = -dif;
	return ArtifactID(id+dif);
}

CreatureID CArtHandler::machineIDToCreature(ArtifactID id)
{
	int dif = 142;

	switch (id)
	{
	case 6:
		dif--;
		break;
	case 5:
		dif++;
		break;
	}
	return CreatureID(id + dif);
}

ArtifactID CArtHandler::getRandomArt(int flags)
{
	return getArtSync(ran(), flags, true);
}
ArtifactID CArtHandler::getArtSync (ui32 rand, int flags, bool erasePicked)
{
	auto getAllowedArts = [&](std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, CArtifact::EartClass flag)
	{
		if (arts->empty()) //restock available arts
			fillList(*arts, flag);

		for (int i = 0; i < arts->size(); ++i)
		{
			CArtifact *art = (*arts)[i];
			out.push_back(art);
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

		//tlog0 << "Treasure count: " << treasures.size() << std::endl;
	};

	std::vector<ConstTransitivePtr<CArtifact> > out;
	getAllowed(out);
	ArtifactID artID = out[rand % out.size()]->id;
	if(erasePicked)
		erasePickedArt (artID);
	return artID;
}

Bonus *createBonus(Bonus::BonusType type, int val, int subtype, Bonus::ValueType valType, shared_ptr<ILimiter> limiter = shared_ptr<ILimiter>(), int additionalInfo = 0)
{
	Bonus *added = new Bonus(Bonus::PERMANENT,type,Bonus::ARTIFACT,val,-1,subtype);
	added->additionalInfo = additionalInfo;
	added->valType = valType;
	added->limiter = limiter;
	return added;
}

Bonus *createBonus(Bonus::BonusType type, int val, int subtype, shared_ptr<IPropagator> propagator = shared_ptr<IPropagator>(), int additionalInfo = 0)
{
	Bonus *added = new Bonus(Bonus::PERMANENT,type,Bonus::ARTIFACT,val,-1,subtype);
	added->additionalInfo = additionalInfo;
	added->valType = Bonus::BASE_NUMBER;
	added->propagator = propagator;
	return added;
}

void CArtHandler::giveArtBonus( ArtifactID aid, Bonus::BonusType type, int val, int subtype, Bonus::ValueType valType, shared_ptr<ILimiter> limiter, int additionalInfo)
{
	giveArtBonus(aid, createBonus(type, val, subtype, valType, limiter, additionalInfo));
}

void CArtHandler::giveArtBonus(ArtifactID aid, Bonus::BonusType type, int val, int subtype, shared_ptr<IPropagator> propagator /*= NULL*/, int additionalInfo)
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
	return (art->possibleSlots[ArtBearer::HERO].size() ||
			(art->possibleSlots[ArtBearer::COMMANDER].size() && VLC->modh->modules.COMMANDERS) ||
			(art->possibleSlots[ArtBearer::CREATURE].size() && VLC->modh->modules.STACK_ARTIFACT)) &&
			!(art->constituents); //no combo artifacts spawning
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
		if (allowed[i] && legalArtifact(i))
			allowedArtifacts.push_back(artifacts[i]);
	}
	if (VLC->modh->modules.COMMANDERS) //allow all commander artifacts for testing
	{
		for (int i = 146; i <= 155; ++i)
		{
			allowedArtifacts.push_back(artifacts[i]);
		}
	}
	for (int i = GameConstants::ARTIFACTS_QUANTITY; i < artifacts.size(); ++i) //allow all new artifacts by default
	{
		if (legalArtifact(ArtifactID(i)))
			allowedArtifacts.push_back(artifacts[i]);
			 //keep im mind that artifact can be worn by more than one type of bearer
	}
}

std::vector<bool> CArtHandler::getDefaultAllowedArtifacts() const
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
			tlog2 << "Problem: cannot erase artifact " << art->Name() << " from list, it was not present\n";

	}
	else
		tlog2 << "Problem: cannot find list for artifact " << art->Name() << ", strange class. (special?)\n";
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
		return 0;
	}
}

void CArtHandler::fillList( std::vector<CArtifact*> &listToBeFilled, CArtifact::EartClass artifactClass )
{
	assert(listToBeFilled.empty());
	for (int i = 0; i < allowedArtifacts.size(); ++i)
	{
		if (allowedArtifacts[i]->aClass == artifactClass)
			listToBeFilled.push_back(allowedArtifacts[i]);
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
	CArtifactInstance *ret = new CArtifactInstance(VLC->arth->artifacts[1]);
	Bonus *b = new Bonus(Bonus::PERMANENT, Bonus::SPELL, Bonus::ARTIFACT_INSTANCE, -1, 1, s->id);
	ret->addNewBonus(b);
	return ret;
}

void CArtifactInstance::init()
{
	id = ArtifactInstanceID();
	id = static_cast<ArtifactInstanceID>(ArtifactID::NONE); //to be randomized
	setNodeType(ARTIFACT_INSTANCE);
}

ArtifactPosition CArtifactInstance::firstAvailableSlot(const CArtifactSet *h) const
{
	BOOST_FOREACH(auto slot, artType->possibleSlots[h->bearerType()])
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

bool CArtifactInstance::canBePutAt(const ArtifactLocation al, bool assumeDestRemoved /*= false*/) const
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
		tlog3 << "Warning: artifact " << artType->Name() << " doesn't have defined allowed slots for bearer of type "
			<< artSet->bearerType() << std::endl;
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
	return artType->constituents != nullptr;
}

std::vector<const CArtifact *> CArtifactInstance::assemblyPossibilities(const CArtifactSet *h) const
{
	std::vector<const CArtifact *> ret;
	if(artType->constituents) //combined artifact already: no combining of combined artifacts... for now.
		return ret;

	BOOST_FOREACH(const CArtifact * artifact, artType->constituentOf)
	{
		assert(artifact->constituents);
		bool possible = true;

		BOOST_FOREACH(const CArtifact * constituent, *artifact->constituents) //check if all constituents are available
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
		auto ret = new CArtifactInstance(Art);
		if (dynamic_cast<CGrowingArtifact *>(Art))
		{
			Bonus * bonus = new Bonus;
			bonus->type = Bonus::LEVEL_COUNTER;
			bonus->val = 0;
			ret->addNewBonus (bonus);
		}
		return ret;
	}
	else
	{
		CCombinedArtifactInstance * ret = new CCombinedArtifactInstance(Art);
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
		tlog3 << "Warning: " << nodeName() << " doesn't bear any spell!\n";
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
	BOOST_FOREACH(const ConstituentInfo &constituent, constituentsInfo)
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

	BOOST_FOREACH(const CArtifact * art, *artType->constituents)
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
		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
			ci.slot = ArtifactPosition::PRE_FIRST;
	}
	else
	{
		CArtifactInstance *mainConstituent = figureMainConstituent(al); //it'll be replaced with combined artifact, not a lock
		CArtifactInstance::putAt(al); //puts combined art (this)

		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
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
		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
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
	CArtifactInstance *mainConstituent = NULL; //it'll be replaced with combined artifact, not a lock
	BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
		if(ci.slot == al.slot)
			mainConstituent = ci.art;

	if(!mainConstituent)
	{
		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
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
	BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
		attachTo(ci.art);
}

bool CCombinedArtifactInstance::isPart(const CArtifactInstance *supposedPart) const
{
	bool me = CArtifactInstance::isPart(supposedPart);
	if(me)
		return true;

	//check for constituents
	BOOST_FOREACH(const ConstituentInfo &constituent, constituentsInfo)
		if(constituent.art == supposedPart)
			return true;

	return false;
}

CCombinedArtifactInstance::ConstituentInfo::ConstituentInfo(CArtifactInstance *Art /*= NULL*/, ArtifactPosition Slot /*= -1*/)
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

	return NULL;
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
	BOOST_FOREACH(auto i, artifactsWorn)
		if(i.second.artifact == art)
			return i.first;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact == art)
			return ArtifactPosition(GameConstants::BACKPACK_START + i);

	return ArtifactPosition::PRE_FIRST;
}

const CArtifactInstance * CArtifactSet::getArtByInstanceId( ArtifactInstanceID artInstId ) const
{
	BOOST_FOREACH(auto i, artifactsWorn)
		if(i.second.artifact->id == artInstId)
			return i.second.artifact;

	BOOST_FOREACH(auto i, artifactsInBackpack)
		if(i.artifact->id == artInstId)
			return i.artifact;

	return NULL;
}

bool CArtifactSet::hasArt(ui32 aid, bool onlyWorn /*= false*/) const
{
	return getArtPos(aid, onlyWorn) != ArtifactPosition::PRE_FIRST;
}

const ArtSlotInfo * CArtifactSet::getSlot(ArtifactPosition pos) const
{
	if(vstd::contains(artifactsWorn, pos))
		return &artifactsWorn[pos];
	if(pos >= ArtifactPosition::AFTER_LAST )
	{
		int backpackPos = (int)pos - GameConstants::BACKPACK_START;
		if(backpackPos < 0 || backpackPos >= artifactsInBackpack.size())
			return NULL;
		else
			return &artifactsInBackpack[backpackPos];
	}

	return NULL;
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
		tlog2 << (dynamic_cast<const CGHeroInstance*>(this))->name << " has no artifact at " << pos << " (getArtTypeId)\n";
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
	for(auto i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact && !i->second.locked)
			node->attachTo(i->second.artifact);
}
