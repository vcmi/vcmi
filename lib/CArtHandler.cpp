#include "StdInc.h"
#include "CArtHandler.h"

#include "Filesystem/CResourceLoader.h"
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

const std::map<std::string, CArtifact::EartClass> artifactClassMap = boost::assign::map_list_of 
	("TREASURE", CArtifact::ART_TREASURE)
	("MINOR", CArtifact::ART_MINOR)
	("MAJOR", CArtifact::ART_MAJOR)
	("RELIC", CArtifact::ART_RELIC)
	("SPECIAL", CArtifact::ART_SPECIAL);

#define ART_BEARER(x) ( #x, ArtBearer::x )
	const std::map<std::string, int> artifactBearerMap = boost::assign::map_list_of ART_BEARER_LIST;
#undef ART_BEARER

#define ART_POS(x) ( #x, ArtifactPosition::x )

const std::map<std::string, int> artifactPositionMap = boost::assign::map_list_of 
	ART_POS(HEAD)
	ART_POS(SHOULDERS)
	ART_POS(NECK)
	ART_POS(RIGHT_HAND)
	ART_POS(LEFT_HAND)
	ART_POS(TORSO)
	ART_POS(RIGHT_RING)
	ART_POS(LEFT_RING)
	ART_POS(FEET)
	ART_POS(MISC1)
	ART_POS(MISC2)
	ART_POS(MISC3)
	ART_POS(MISC4)
	ART_POS(MISC5)
	ART_POS(MACH1)
	ART_POS(MACH2)
	ART_POS(MACH3)
	ART_POS(MACH4)
	ART_POS(SPELLBOOK); //no need to specify commander / stack position?

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

// /**
//  * Checks whether the artifact fits at a given slot.
//  * @param artifWorn A hero's set of worn artifacts.
//  */
// bool CArtifact::fitsAt (const std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID) const
// {
// 	if (!vstd::contains(possibleSlots, slotID))
// 		return false;
//
// 	// Can't put an artifact in a locked slot.
// 	std::map<ui16, const CArtifact*>::const_iterator it = artifWorn.find(slotID);
// 	if (it != artifWorn.end() && it->second->id == 145)
// 		return false;
//
// 	// Check if a combination artifact fits.
// 	// TODO: Might want a more general algorithm?
// 	//       Assumes that misc & rings fits only in their slots, and others in only one slot and no duplicates.
// 	if (constituents != NULL)
// 	{
// 		std::map<ui16, const CArtifact*> tempArtifWorn = artifWorn;
// 		const ui16 ringSlots[] = {6, 7};
// 		const ui16 miscSlots[] = {9, 10, 11, 12, 18};
// 		int rings = 0;
// 		int misc = 0;
//
// 		VLC->arth->unequipArtifact(tempArtifWorn, slotID);
//
// 		BOOST_FOREACH(ui32 constituentID, *constituents)
// 		{
// 			const CArtifact& constituent = *VLC->arth->artifacts[constituentID];
// 			const int slot = constituent.possibleSlots[0];
//
// 			if (slot == 6 || slot == 7)
// 				rings++;
// 			else if ((slot >= 9 && slot <= 12) || slot == 18)
// 				misc++;
// 			else if (tempArtifWorn.find(slot) != tempArtifWorn.end())
// 				return false;
// 		}
//
// 		// Ensure enough ring slots are free
// 		for (int i = 0; i < sizeof(ringSlots)/sizeof(*ringSlots); i++)
// 		{
// 			if (tempArtifWorn.find(ringSlots[i]) == tempArtifWorn.end() || ringSlots[i] == slotID)
// 				rings--;
// 		}
// 		if (rings > 0)
// 			return false;
//
// 		// Ensure enough misc slots are free.
// 		for (int i = 0; i < sizeof(miscSlots)/sizeof(*miscSlots); i++)
// 		{
// 			if (tempArtifWorn.find(miscSlots[i]) == tempArtifWorn.end() || miscSlots[i] == slotID)
// 				misc--;
// 		}
// 		if (misc > 0)
// 			return false;
// 	}
//
// 	return true;
// }

// bool CArtifact::canBeAssembledTo (const std::map<ui16, const CArtifact*> &artifWorn, ui32 artifactID) const
// {
// 	if (constituentOf == NULL || !vstd::contains(*constituentOf, artifactID))
// 		return false;
//
// 	const CArtifact &artifact = *VLC->arth->artifacts[artifactID];
// 	assert(artifact.constituents);
//
// 	BOOST_FOREACH(ui32 constituentID, *artifact.constituents)
// 	{
// 		bool found = false;
// 		for (std::map<ui16, const CArtifact*>::const_iterator it = artifWorn.begin(); it != artifWorn.end(); ++it)
// 		{
// 			if (it->second->id == constituentID)
// 			{
// 				found = true;
// 				break;
// 			}
// 		}
// 		if (!found)
// 			return false;
// 	}
//
// 	return true;
// }

CArtifact::CArtifact()
{
	setNodeType(ARTIFACT);
	possibleSlots[ArtBearer::HERO]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::CREATURE]; //we want to generate map entry even if it will be empty
	possibleSlots[ArtBearer::COMMANDER];
	constituents = NULL; //default pointer to zero
	constituentOf = NULL;
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
// void CArtifact::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
// {
// 	//combined artifact carries bonuses from its parts
// 	if(constituents)
// 	{
// 		BOOST_FOREACH(ui32 id, *constituents)
// 			out.insert(VLC->arth->artifacts[id]);
// 	}
// }

// void CScroll::Init()
// {
// // 	addNewBonus (Bonus (Bonus::PERMANENT, Bonus::SPELL, Bonus::ARTIFACT, 1, id, spellid, Bonus::INDEPENDENT_MAX));
// // 	//boost::algorithm::replace_first(description, "[spell name]", VLC->spellh->spells[spellid].name);
// }

void CArtifact::addNewBonus(Bonus *b)
{
	b->source = Bonus::ARTIFACT;
	b->duration = Bonus::PERMANENT;
	b->description = name;
	CBonusSystemNode::addNewBonus(b);
}

void CArtifact::setName (std::string desc)
{
	name = desc;
}
void CArtifact::setDescription (std::string desc)
{
	description = desc;
}
void CArtifact::setEventText (std::string desc)
{
	eventText = desc;
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
	for (ui32 i = 3; i <= 6; i++)
		bigArtifacts.insert(i);
}

CArtHandler::~CArtHandler()
{
	for (std::vector< ConstTransitivePtr<CArtifact> >::iterator it = artifacts.begin(); it != artifacts.end(); ++it)
	{
		delete (*it)->constituents;
		delete (*it)->constituentOf;
	}
}

void CArtHandler::loadArtifacts(bool onlyTxt)
{
	std::vector<ui16> slots;
	slots += 17, 16, 15, 14, 13, 18, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0;
	growingArtifacts += 146, 147, 148, 150, 151, 152, 153;
	static std::map<char, CArtifact::EartClass> classes =
	  map_list_of('S',CArtifact::ART_SPECIAL)('T',CArtifact::ART_TREASURE)('N',CArtifact::ART_MINOR)('J',CArtifact::ART_MAJOR)('R',CArtifact::ART_RELIC);

	CLegacyConfigParser parser("DATA/ARTRAITS.TXT");
	CLegacyConfigParser events("DATA/ARTEVENT.TXT");

	parser.endLine(); // header
	parser.endLine();

	std::map<ui32,ui8>::iterator itr;

	for (int i=0; i<GameConstants::ARTIFACTS_QUANTITY; i++)
	{
		CArtifact *art;
		if (vstd::contains (growingArtifacts, i))
		{
			art = new CGrowingArtifact();
		}
		else
		{
			art = new CArtifact();
		}
		CArtifact &nart = *art;
		nart.id=i;
		nart.iconIndex=i;
		nart.setName (parser.readString());
		nart.setEventText (events.readString());
		events.endLine();

		nart.price= parser.readNumber();

		for(int j=0;j<slots.size();j++)
		{
			if(parser.readString() == "x")
				nart.possibleSlots[ArtBearer::HERO].push_back(slots[j]);
		}
		nart.aClass = classes[parser.readString()[0]];

		//load description and remove quotation marks
		nart.setDescription (parser.readString());

		parser.endLine();

		if(onlyTxt)
			continue;

		artifacts.push_back(&nart);
	}
	if (VLC->modh->modules.COMMANDERS)
	{ //TODO: move all artifacts config to separate json file
		const JsonNode config(ResourceID("config/commanders.json"));
		BOOST_FOREACH(const JsonNode &artifact, config["artifacts"].Vector())
		{
			auto ga = dynamic_cast <CGrowingArtifact *>(artifacts[artifact["id"].Float()].get());
			BOOST_FOREACH (auto b, artifact["bonusesPerLevel"].Vector())
			{
				ga->bonusesPerLevel.push_back (std::pair <ui16, Bonus> (b["level"].Float(), *JsonUtils::parseBonus (b["bonus"].Vector())));
			}
			BOOST_FOREACH (auto b, artifact["thresholdBonuses"].Vector())
			{
				ga->thresholdBonuses.push_back (std::pair <ui16, Bonus> (b["level"].Float(), *JsonUtils::parseBonus (b["bonus"].Vector())));
			}
		}
	}

	sortArts();
	if(onlyTxt)
		return;

	addBonuses();

	// Populate reverse mappings of combinational artifacts.
	//TODO: do that also for new artifacts read from mods
	BOOST_FOREACH(CArtifact *artifact, artifacts)
	{
		if (artifact->constituents != NULL)
		{
			BOOST_FOREACH(ui32 constituentID, *artifact->constituents)
			{
				if (artifacts[constituentID]->constituentOf == NULL)
					artifacts[constituentID]->constituentOf = new std::vector<ui32>();
				artifacts[constituentID]->constituentOf->push_back(artifact->id);
			}
		}
	}
}

void CArtHandler::load(const JsonNode & node)
{
	BOOST_FOREACH(auto & entry, node.Struct())
	{
		if (!entry.second.isNull()) // may happens if mod removed creature by setting json entry to null
		{
			CArtifact * art = loadArtifact(entry.second);
			art->id = artifacts.size();

			artifacts.push_back(art);
			tlog5 << "Added artifact: " << entry.first << "\n";
			VLC->modh->identifiers.registerObject (std::string("artifact.") + entry.first, art->id);
		}
	}
}

CArtifact * CArtHandler::loadArtifact(const JsonNode & node)
{
	CArtifact * art = new CArtifact;
	const JsonNode *value;

	const JsonNode & text = node["text"];
	art->setName		(text["name"].String());
	art->setDescription	(text["description"].String());
	art->setEventText		(text["event"].String());

	const JsonNode & graphics = node["graphics"];
	art->iconIndex = graphics["iconIndex"].Float();
	art->image = graphics["image"].String();
	value = &graphics["large"];
	if (!value->isNull())
		art->large = value->String();
	art->advMapDef = graphics["map"].String();

	art->price = node["value"].Float();

	{
		auto it = artifactClassMap.find (node["class"].String());
		if (it != artifactClassMap.end())
		{
			art->aClass = it->second;
		}
		else
		{
			tlog2 << "Warning! Artifact rarity " << value->String() << " not recognized!";
			art->aClass = CArtifact::ART_SPECIAL;
		}
	}
	
	int bearerType = -1;
	bool heroArt = false;

	{
		const JsonNode & bearer = node["type"];
		BOOST_FOREACH (const JsonNode & b, bearer.Vector())
		{
			auto it = artifactBearerMap.find (b.String());
			if (it != artifactBearerMap.end())
			{
				bearerType = it->second;
				switch (bearerType)
				{
					case ArtBearer::HERO: //TODO: allow arts having several possible bearers
						heroArt = true;
						break;
					case ArtBearer::COMMANDER:
						makeItCommanderArt (art, false); //do not erase already existing slots
						break;
					case ArtBearer::CREATURE:
						makeItCreatureArt (art, false);
						break;
				}
			}
			else
				tlog2 << "Warning! Artifact type " << b.String() << " not recognized!";
		}
	}

	value = &node["slot"];
	if (!value->isNull() && heroArt) //we assume non-hero slots are irrelevant?
	{
		std::string slotName = value->String();
		if (slotName == "MISC")
		{
			//unfortunatelly slot ids aare not continuous
			art->possibleSlots[ArtBearer::HERO] += ArtifactPosition::MISC1, ArtifactPosition::MISC2, ArtifactPosition::MISC3, ArtifactPosition::MISC4, ArtifactPosition::MISC5;
		}
		else if (slotName == "RING")
		{
			art->possibleSlots[ArtBearer::HERO] += ArtifactPosition::LEFT_RING, ArtifactPosition::RIGHT_RING;
		}
		else
		{

			auto it = artifactPositionMap.find (slotName);
			if (it != artifactPositionMap.end())
			{
				auto slot = it->second;
				art->possibleSlots[ArtBearer::HERO].push_back (slot);
			}
			else
				tlog2 << "Warning! Artifact slot " << value->String() << " not recognized!";
		}
	}

	readComponents (node, art);

	BOOST_FOREACH (const JsonNode &bonus, node["bonuses"].Vector())
	{
		auto b = JsonUtils::parseBonus(bonus);
		//TODO: bonus->sid = art->id;
		art->addNewBonus(b);
	}

	return art;
}

void CArtifact::addConstituent (ui32 component)
{
	assert (constituents);
	constituents->push_back (component);
}

int CArtHandler::convertMachineID(int id, bool creToArt )
{
	int dif = 142;
	if(creToArt)
	{
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
	}
	else
	{
		switch (id)
		{
		case 6:
			dif--;
			break;
		case 5:
			dif++;
			break;
		}
	}
	return id + dif;
}

void CArtHandler::sortArts()
{
 	//for (int i=0; i<allowedArtifacts.size(); ++i) //do 144, bo nie chcemy bzdurek
 	//{
 	//	switch (allowedArtifacts[i]->aClass)
 	//	{
 	//	case CArtifact::ART_TREASURE:
 	//		treasures.push_back(allowedArtifacts[i]);
 	//		break;
 	//	case CArtifact::ART_MINOR:
 	//		minors.push_back(allowedArtifacts[i]);
 	//		break;
 	//	case CArtifact::ART_MAJOR:
 	//		majors.push_back(allowedArtifacts[i]);
 	//		break;
 	//	case CArtifact::ART_RELIC:
 	//		relics.push_back(allowedArtifacts[i]);
 	//		break;
 	//	}
 	//}
}
void CArtHandler::erasePickedArt( TArtifactInstanceID id )
{
	std::vector<CArtifact*>* ptr;
	CArtifact *art = artifacts[id];
	switch (art->aClass)
	{
		case CArtifact::ART_TREASURE:
			ptr = &treasures;
			break;
		case CArtifact::ART_MINOR:
			ptr = &minors;
			break;
		case CArtifact::ART_MAJOR:
			ptr = &majors;
			break;
		case CArtifact::ART_RELIC:
			ptr = &relics;
			break;
		default: //special artifacts should not be erased
			return;
	}
	ptr->erase (std::find(ptr->begin(), ptr->end(), art)); //remove the artifact from available list
}
ui16 CArtHandler::getRandomArt(int flags)
{
	std::vector<ConstTransitivePtr<CArtifact> > out;
	getAllowed(out, flags);
	ui16 id = out[ran() % out.size()]->id;
	erasePickedArt (id);
	return id;
}
ui16 CArtHandler::getArtSync (ui32 rand, int flags)
{
	std::vector<ConstTransitivePtr<CArtifact> > out;
	getAllowed(out, flags);
	CArtifact *art = out[rand % out.size()];
	return art->id;
}
void CArtHandler::getAllowed(std::vector<ConstTransitivePtr<CArtifact> > &out, int flags)
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
}
void CArtHandler::getAllowedArts(std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, int flag)
{
	if (arts->empty()) //restock available arts
	{
		for (int i = 0; i < allowedArtifacts.size(); ++i)
		{
			if (allowedArtifacts[i]->aClass == flag)
				arts->push_back(allowedArtifacts[i]);
		}
	}

	for (int i = 0; i < arts->size(); ++i)
	{
		CArtifact *art = (*arts)[i];
		out.push_back(art);
	}
}

Bonus *createBonus(Bonus::BonusType type, int val, int subtype, int valType, shared_ptr<ILimiter> limiter = shared_ptr<ILimiter>(), int additionalInfo = 0)
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

void CArtHandler::giveArtBonus( TArtifactID aid, Bonus::BonusType type, int val, int subtype, int valType, shared_ptr<ILimiter> limiter, int additionalInfo)
{
	giveArtBonus(aid, createBonus(type, val, subtype, valType, limiter, additionalInfo));
}

void CArtHandler::giveArtBonus(TArtifactID aid, Bonus::BonusType type, int val, int subtype, shared_ptr<IPropagator> propagator /*= NULL*/, int additionalInfo)
{
	giveArtBonus(aid, createBonus(type, val, subtype, propagator, additionalInfo));
}

void CArtHandler::giveArtBonus(TArtifactID aid, Bonus *bonus)
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

void CArtHandler::makeItCreatureArt (TArtifactInstanceID aid, bool onlyCreature /*=true*/)
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
		a->possibleSlots[ArtBearer::COMMANDER].push_back(i);
}

void CArtHandler::makeItCommanderArt( TArtifactInstanceID aid, bool onlyCommander /*= true*/ )
{
	CArtifact *a = artifacts[aid];
	makeItCommanderArt (a, onlyCommander);
}

void CArtHandler::addBonuses()
{
	const JsonNode config(ResourceID("config/artifacts.json"));
	BOOST_FOREACH (auto & artifact, config["artifacts"].Struct()) //pair <string, JsonNode> (id, properties)
	{
		auto ga = artifacts[artifact.second["id"].Float()].get();
		
		BOOST_FOREACH (auto b, artifact.second["bonuses"].Vector())
		{
			auto bonus = JsonUtils::parseBonus (b);
			bonus->sid = ga->id;
			ga->addNewBonus (bonus);
		}
		BOOST_FOREACH (const JsonNode & b, artifact.second["type"].Vector()) //TODO: remove duplicate code
		{
			auto it = artifactBearerMap.find (b.String());
			if (it != artifactBearerMap.end())
			{
				int bearerType = it->second;
				switch (bearerType)
				{
					case ArtBearer::HERO:
						break;
					case ArtBearer::COMMANDER:
						makeItCommanderArt (ga); //original artifacts should have only one bearer type
						break;
					case ArtBearer::CREATURE:
						makeItCreatureArt (ga);
						break;
				}
			}
			else
				tlog2 << "Warning! Artifact type " << b.String() << " not recognized!";
		}
		readComponents (artifact.second, ga);

		VLC->modh->identifiers.registerObject ("artifact." + artifact.first, ga->id);
	}
}

void CArtHandler::readComponents (const JsonNode & node, CArtifact * art)
{
	const JsonNode *value;
	value = &node["components"];
	if (!value->isNull())
	{
		art->constituents = new std::vector<ui32>();
		BOOST_FOREACH (auto component, value->Vector())
		{
			VLC->modh->identifiers.requestIdentifier(std::string("artifact.") + component.String(),
				boost::bind (&CArtifact::addConstituent, art, _1)
			);
		}
	}

}

void CArtHandler::clear()
{
	BOOST_FOREACH(CArtifact *art, artifacts)
		delete art;
	artifacts.clear();

	clearHlpLists();

}

void CArtHandler::clearHlpLists()
{
	treasures.clear();
	minors.clear();
	majors.clear();
	relics.clear();
}

bool CArtHandler::legalArtifact(int id)
{
	return (artifacts[id]->possibleSlots[ArtBearer::HERO].size() ||
			(artifacts[id]->possibleSlots[ArtBearer::COMMANDER].size() && VLC->modh->modules.COMMANDERS)) ||
			(artifacts[id]->possibleSlots[ArtBearer::CREATURE].size() && VLC->modh->modules.STACK_ARTIFACT);
}

void CArtHandler::initAllowedArtifactsList(const std::vector<ui8> &allowed)
{
	allowedArtifacts.clear();
	clearHlpLists();
	for (int i=0; i<144; ++i) //yes, 144
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
		 if (artifacts[i]->possibleSlots[ArtBearer::HERO].size())
			allowedArtifacts.push_back(artifacts[i]);
		 else //check if active modules allow artifact to be every used
		 {
			 if (legalArtifact(i))
				 allowedArtifacts.push_back(artifacts[i]);
			 //keep im mind that artifact can be worn by more than one type of bearer
		 }
	}
}

std::vector<ui8> CArtHandler::getDefaultAllowedArtifacts() const
{
	std::vector<ui8> allowedArtifacts;
	allowedArtifacts.resize(127, 1);
	allowedArtifacts.resize(141, 0);
	allowedArtifacts.resize(GameConstants::ARTIFACTS_QUANTITY, 1);
	return allowedArtifacts;
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

// CArtifactInstance::CArtifactInstance(int aid)
// {
// 	init();
// 	setType(VLC->arth->artifacts[aid]);
// }

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
	id = -1;
	setNodeType(ARTIFACT_INSTANCE);
}

int CArtifactInstance::firstAvailableSlot(const CArtifactSet *h) const
{
	BOOST_FOREACH(ui16 slot, artType->possibleSlots[h->bearerType()])
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

int CArtifactInstance::firstBackpackSlot(const CArtifactSet *h) const
{
	if(!artType->isBig()) //discard big artifact
		return GameConstants::BACKPACK_START + h->artifactsInBackpack.size();

	return -1;
}

bool CArtifactInstance::canBePutAt(const ArtifactLocation al, bool assumeDestRemoved /*= false*/) const
{
	return canBePutAt(al.getHolderArtSet(), al.slot, assumeDestRemoved);
}

bool CArtifactInstance::canBePutAt(const CArtifactSet *artSet, int slot, bool assumeDestRemoved /*= false*/) const
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
		tlog3 << "Warning: arrtifact " << artType->Name() << " doesn't have defined allowed slots for bearer of type "
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
	return artType->constituents && artType->constituentOf->size();
}

std::vector<const CArtifact *> CArtifactInstance::assemblyPossibilities(const CArtifactSet *h) const
{
	std::vector<const CArtifact *> ret;
	if(!artType->constituentOf //not a part of combined artifact
		|| artType->constituents) //combined artifact already: no combining of combined artifacts... for now.
		return ret;

	BOOST_FOREACH(ui32 possibleCombinedArt, *artType->constituentOf)
	{
		const CArtifact * const artifact = VLC->arth->artifacts[possibleCombinedArt];
		assert(artifact->constituents);
		bool possible = true;

		BOOST_FOREACH(ui32 constituentID, *artifact->constituents) //check if all constituents are available
		{
			if(!h->hasArt(constituentID, true)) //constituent must be equipped
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

int CArtifactInstance::getGivenSpellID() const
{
	const Bonus * b = getBonusLocalFirst(Selector::type(Bonus::SPELL));
	if(!b)
	{
		tlog3 << "Warning: " << nodeName() << " doesn't bear any spell!\n";
		return -1;
	}
	return b->subtype;
}

bool CArtifactInstance::isPart(const CArtifactInstance *supposedPart) const
{
	return supposedPart == this;
}

bool CCombinedArtifactInstance::canBePutAt(const CArtifactSet *artSet, int slot, bool assumeDestRemoved /*= false*/) const
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
		for(std::vector<ConstituentInfo>::iterator art = constituentsToBePlaced.begin(); art != constituentsToBePlaced.end(); art++)
		{
			if(art->art->canBePutAt(artSet, i, i == slot)) // i == al.slot because we can remove already worn artifact only from that slot  that is our main destination
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

	BOOST_FOREACH(ui32 a, *artType->constituents)
	{
		addAsConstituent(CArtifactInstance::createNewArtifactInstance(a), -1);
	}
}

void CCombinedArtifactInstance::addAsConstituent(CArtifactInstance *art, int slot)
{
	assert(vstd::contains(*artType->constituents, art->artType->id));
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
			ci.slot = -1;
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

				int pos = -1;
				if(inActiveSlot  &&  suggestedPosValid) //there is a valid suggestion where to place lock
					pos = ci.slot;
				else
					ci.slot = pos = ci.art->firstAvailableSlot(al.getHolderArtSet());

				assert(pos < GameConstants::BACKPACK_START);
				al.getHolderArtSet()->setNewArtSlot(pos, ci.art, true); //sets as lock
			}
			else
			{
				ci.slot = -1;
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
				ci.slot = -1;
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

CCombinedArtifactInstance::ConstituentInfo::ConstituentInfo(CArtifactInstance *Art /*= NULL*/, ui16 Slot /*= -1*/)
{
	art = Art;
	slot = Slot;
}

bool CCombinedArtifactInstance::ConstituentInfo::operator==(const ConstituentInfo &rhs) const
{
	return art == rhs.art && slot == rhs.slot;
}

const CArtifactInstance* CArtifactSet::getArt(ui16 pos, bool excludeLocked /*= true*/) const
{
	if(const ArtSlotInfo *si = getSlot(pos))
	{
		if(si->artifact && (!excludeLocked || !si->locked))
			return si->artifact;
	}

	return NULL;
}

CArtifactInstance* CArtifactSet::getArt(ui16 pos, bool excludeLocked /*= true*/)
{
	return const_cast<CArtifactInstance*>((const_cast<const CArtifactSet*>(this))->getArt(pos, excludeLocked));
}

si32 CArtifactSet::getArtPos(int aid, bool onlyWorn /*= true*/) const
{
	for(std::map<ui16, ArtSlotInfo>::const_iterator i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact->artType->id == aid)
			return i->first;

	if(onlyWorn)
		return -1;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact->artType->id == aid)
			return GameConstants::BACKPACK_START + i;

	return -1;
}

si32 CArtifactSet::getArtPos(const CArtifactInstance *art) const
{
	for(std::map<ui16, ArtSlotInfo>::const_iterator i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact == art)
			return i->first;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact == art)
			return GameConstants::BACKPACK_START + i;

	return -1;
}

const CArtifactInstance * CArtifactSet::getArtByInstanceId( TArtifactInstanceID artInstId ) const
{
	for(std::map<ui16, ArtSlotInfo>::const_iterator i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact->id == artInstId)
			return i->second.artifact;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact->id == artInstId)
			return artifactsInBackpack[i].artifact;

	return NULL;
}

bool CArtifactSet::hasArt(ui32 aid, bool onlyWorn /*= false*/) const
{
	return getArtPos(aid, onlyWorn) != -1;
}

const ArtSlotInfo * CArtifactSet::getSlot(ui16 pos) const
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

bool CArtifactSet::isPositionFree(ui16 pos, bool onlyLockCheck /*= false*/) const
{
	if(const ArtSlotInfo *s = getSlot(pos))
		return (onlyLockCheck || !s->artifact) && !s->locked;

	return true; //no slot means not used
}

si32 CArtifactSet::getArtTypeId(ui16 pos) const
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

ArtSlotInfo & CArtifactSet::retreiveNewArtSlot(ui16 slot)
{
	assert(!vstd::contains(artifactsWorn, slot));
	ArtSlotInfo &ret = slot < GameConstants::BACKPACK_START
		? artifactsWorn[slot]
		: *artifactsInBackpack.insert(artifactsInBackpack.begin() + (slot - GameConstants::BACKPACK_START), ArtSlotInfo());

	return ret;
}

void CArtifactSet::setNewArtSlot(ui16 slot, CArtifactInstance *art, bool locked)
{
	ArtSlotInfo &asi = retreiveNewArtSlot(slot);
	asi.artifact = art;
	asi.locked = locked;
}

void CArtifactSet::eraseArtSlot(ui16 slot)
{
	if(slot < GameConstants::BACKPACK_START)
	{
		artifactsWorn.erase(slot);
	}
	else
	{
		slot -= GameConstants::BACKPACK_START;
		artifactsInBackpack.erase(artifactsInBackpack.begin() + slot);
	}
}

void CArtifactSet::artDeserializationFix(CBonusSystemNode *node)
{
	for(bmap<ui16, ArtSlotInfo>::iterator i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact && !i->second.locked)
			node->attachTo(i->second.artifact);
}
