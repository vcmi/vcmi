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

#include "ArtifactUtils.h"

#include "../../CCreatureHandler.h"
#include "../../ExceptionsCommon.h"
#include "../../GameLibrary.h"
#include "../../IGameSettings.h"
#include "../../json/JsonBonus.h"

#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../modding/IdentifierStorage.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../../texts/CLegacyConfigParser.h"

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

CArtHandler::~CArtHandler() = default;

std::vector<JsonNode> CArtHandler::loadLegacyData()
{
	size_t dataSize = LIBRARY->engineSettings()->getInteger(EGameSettings::TEXTS_ARTIFACT);

	objects.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	#define ART_POS(x) #x ,
	const std::vector<std::string> artSlots = { ART_POS_LIST };
	#undef ART_POS

	static const std::map<char, std::string> classes =
		{{'S',"SPECIAL"}, {'T',"TREASURE"},{'N',"MINOR"},{'J',"MAJOR"},{'R',"RELIC"},};

	CLegacyConfigParser parser(TextPath::builtin("DATA/ARTRAITS.TXT"));
	CLegacyConfigParser events(TextPath::builtin("DATA/ARTEVENT.TXT"));

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
				artData["slot"].Vector().emplace_back(artSlot);
			}
		}

		std::string artClass = parser.readString();

		if (classes.count(artClass[0]))
			artData["class"].String() = classes.at(artClass[0]);
		else
			throw DataLoadingException("File ARTRAITS.TXT is invalid or corrupted! Please reinstall Heroes III data files");
		artData["text"]["description"].String() = parser.readString();

		parser.endLine();
		events.endLine();
		h3Data.push_back(artData);
	}
	return h3Data;
}

void CArtHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(scope, data, name, objects.size());

	object->iconIndex = object->getIndex() + 5;

	objects.emplace_back(object);

	registerObject(scope, "artifact", name, object->id.getNum());
}

void CArtHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(scope, data, name, index);

	object->iconIndex = object->getIndex();

	assert(objects[index] == nullptr); // ensure that this id was not loaded before
	objects[index] = object;

	registerObject(scope, "artifact", name, object->id.getNum());
}

const std::vector<std::string> & CArtHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "artifact" };
	return typeNames;
}

std::shared_ptr<CArtifact> CArtHandler::loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	auto art = std::make_shared<CArtifact>();
	if(!node["growing"].isNull())
	{
		for(auto bonus : node["growing"]["bonusesPerLevel"].Vector())
			art->bonusesPerLevel.emplace_back(static_cast<ui16>(bonus["level"].Float()), JsonUtils::parseBonus(bonus["bonus"]));

		for(auto bonus : node["growing"]["thresholdBonuses"].Vector())
			art->thresholdBonuses.emplace_back(static_cast<ui16>(bonus["level"].Float()), JsonUtils::parseBonus(bonus["bonus"]));
	}

	art->id = ArtifactID(index);
	art->identifier = identifier;
	art->modScope = scope;

	const JsonNode & text = node["text"];

	LIBRARY->generaltexth->registerString(scope, art->getNameTextID(), text["name"]);
	LIBRARY->generaltexth->registerString(scope, art->getDescriptionTextID(), text["description"]);
	LIBRARY->generaltexth->registerString(scope, art->getEventTextID(), text["event"]);

	const JsonNode & graphics = node["graphics"];
	art->image = graphics["image"].String();

	if(!graphics["scenarioBonus"].isNull())
		art->scenarioBonus = graphics["scenarioBonus"].String();
	else
		art->scenarioBonus = art->image; // MOD COMPATIBILITY fallback for pre-1.7 mods

	art->advMapDef = graphics["map"].String();

	art->price = static_cast<ui32>(node["value"].Float());
	art->onlyOnWaterMap = node["onlyOnWaterMap"].Bool();

	loadSlots(art.get(), node);
	loadClass(art.get(), node);
	loadType(art.get(), node);
	loadComponents(art.get(), node);

	if (node["bonuses"].isVector())
	{
		for(const auto & b : node["bonuses"].Vector())
		{
			auto bonus = JsonUtils::parseBonus(b);
			bonus->sid = art->getId();
			art->addNewBonus(bonus);
		}
	}
	else
	{
		for(const auto & b : node["bonuses"].Struct())
		{
			if (b.second.isNull())
				continue;
			auto bonus = JsonUtils::parseBonus(b.second, art->getBonusTextID(b.first));
			bonus->sid = art->getId();
			art->addNewBonus(bonus);
		}
	}

	for(const auto & b : node["instanceBonuses"].Struct())
	{
		if (b.second.isNull())
			continue;
		auto bonus = JsonUtils::parseBonus(b.second, art->getBonusTextID(b.first));
		bonus->sid = art->getId();
		bonus->source = BonusSource::ARTIFACT;
		bonus->duration = BonusDuration::PERMANENT;
		bonus->description.appendTextID(art->getNameTextID());
		bonus->description.appendRawString(" %+d");
		art->instanceBonuses.push_back(bonus);
	}

	const JsonNode & warMachine = node["warMachine"];
	if(!warMachine.isNull())
	{
		LIBRARY->identifiers()->requestIdentifier("creature", warMachine, [art](si32 id)
		{
			art->warMachine = CreatureID(id);

			//this assumes that creature object is stored before registration
			LIBRARY->creh->objects.at(id)->warMachine = art->id;
		});
	}

	LIBRARY->identifiers()->requestIdentifier(scope, "object", "artifact", [scope, art](si32 index)
	{
		JsonNode conf;
		conf.setModScope(scope);

		LIBRARY->objtypeh->loadSubObject(art->identifier, conf, Obj::ARTIFACT, art->getIndex());

		if(!art->advMapDef.empty())
		{
			JsonNode templ;
			templ["animation"].String() = art->advMapDef;
			templ.setModScope(scope);

			// add new template.
			// Necessary for objects added via mods that don't have any templates in H3
			LIBRARY->objtypeh->getHandlerFor(Obj::ARTIFACT, art->getIndex())->addTemplate(templ);
		}
	});

	if(art->isTradable())
		art->possibleSlots.at(ArtBearer::ALTAR).push_back(ArtifactPosition::ALTAR);

	if(!node["charged"].isNull())
	{
		art->setCondition(stringToDischargeCond(node["charged"]["usageType"].String()));
		if(!node["charged"]["removeOnDepletion"].isNull())
			art->setRemoveOnDepletion(node["charged"]["removeOnDepletion"].Bool());
		if(!node["charged"]["startingCharges"].isNull())
		{
			const auto charges = node["charged"]["startingCharges"].Integer();
			if(charges < 0)
				logMod->warn("Warning! Charged artifact %s number of charges cannot be less than zero %d!", art->getNameTranslated(), charges);
			else
				art->setDefaultStartCharges(charges);
		}
	}

	// Some bonuses must be located in the instance.
	for(const auto & b : art->getExportedBonusList())
	{
		if(std::dynamic_pointer_cast<const HasChargesLimiter>(b->limiter))
		{
			b->source = BonusSource::ARTIFACT;
			b->duration = BonusDuration::PERMANENT;
			b->description.appendTextID(art->getNameTextID());
			b->description.appendRawString(" %+d");
			art->instanceBonuses.push_back(b);
			art->removeBonus(b);
		}
	}

	return art;
}

int32_t ArtifactPositionBase::decode(const std::string & slotName)
{
#define ART_POS(x) { #x, ArtifactPosition::x },
	static const std::map<std::string, ArtifactPosition> artifactPositionMap = { ART_POS_LIST };
#undef ART_POS
	auto it = artifactPositionMap.find (slotName);
	if (it != artifactPositionMap.end())
		return it->second;
	else
		return PRE_FIRST;
}

std::string ArtifactPositionBase::encode(int32_t index)
{
#define ART_POS(x) #x ,
	const std::vector<std::string> artSlots = { ART_POS_LIST };
#undef ART_POS

	return artSlots.at(index);
}

std::string ArtifactPositionBase::entityType()
{
	return "artifactSlot";
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
		auto slot = ArtifactPosition::decode(slotID);
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

EArtifactClass CArtHandler::stringToClass(const std::string & className)
{
	static const std::map<std::string, EArtifactClass> artifactClassMap =
	{
		{"TREASURE", EArtifactClass::ART_TREASURE},
		{"MINOR", EArtifactClass::ART_MINOR},
		{"MAJOR", EArtifactClass::ART_MAJOR},
		{"RELIC", EArtifactClass::ART_RELIC},
		{"SPECIAL", EArtifactClass::ART_SPECIAL}
	};

	auto it = artifactClassMap.find (className);
	if (it != artifactClassMap.end())
		return it->second;

	logMod->warn("Warning! Artifact rarity %s not recognized!", className);
	return EArtifactClass::ART_SPECIAL;
}

DischargeArtifactCondition CArtHandler::stringToDischargeCond(const std::string & cond) const
{
	const std::unordered_map<std::string, DischargeArtifactCondition> growingConditionsMap =
	{
		{"SPELLCAST", DischargeArtifactCondition::SPELLCAST},
		{"BATTLE", DischargeArtifactCondition::BATTLE},
		//{"BUILDING", DischargeArtifactCondition::BUILDING},
	};
	return growingConditionsMap.at(cond);
}

void CArtHandler::loadClass(CArtifact * art, const JsonNode & node) const
{
	art->aClass = stringToClass(node["class"].String());
}

void CArtHandler::loadType(CArtifact * art, const JsonNode & node) const
{
#define ART_BEARER(x) { #x, ArtBearer::x },
	static const std::map<std::string, ArtBearer> artifactBearerMap = { ART_BEARER_LIST };
#undef ART_BEARER

	for (const JsonNode & b : node["type"].Vector())
	{
		auto it = artifactBearerMap.find (b.String());
		if (it != artifactBearerMap.end())
		{
			ArtBearer bearerType = it->second;
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
	if(!node["components"].isNull())
	{
		for(const auto & component : node["components"].Vector())
		{
			LIBRARY->identifiers()->requestIdentifier("artifact", component, [this, art](int32_t id)
			{
				// when this code is called both combinational art as well as component are loaded
				// so it is safe to access any of them
				art->constituents.push_back(ArtifactID(id).toArtifact());
				objects[id]->partOf.insert(art);
			});
		}
	}
	if(!node["fusedComponents"].isNull())
		art->setFused(node["fusedComponents"].Bool());
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
	for(const auto & slot : ArtifactUtils::commanderSlots())
		a->possibleSlots[ArtBearer::COMMANDER].push_back(ArtifactPosition(slot));
}

bool CArtHandler::legalArtifact(const ArtifactID & id) const
{
	auto art = id.toArtifact();

	if(art->isCombined())
		return false; //no combo artifacts spawning

	if(art->aClass < EArtifactClass::ART_TREASURE || art->aClass > EArtifactClass::ART_RELIC)
		return false; // invalid class

	if(art->possibleSlots.count(ArtBearer::HERO) && !art->possibleSlots.at(ArtBearer::HERO).empty())
		return true;

	if(art->possibleSlots.count(ArtBearer::CREATURE) && !art->possibleSlots.at(ArtBearer::CREATURE).empty() && LIBRARY->engineSettings()->getBoolean(EGameSettings::MODULE_STACK_ARTIFACT))
		return true;

	if(art->possibleSlots.count(ArtBearer::COMMANDER) && !art->possibleSlots.at(ArtBearer::COMMANDER).empty() && LIBRARY->engineSettings()->getBoolean(EGameSettings::MODULE_COMMANDERS))
		return true;

	return false;
}

std::set<ArtifactID> CArtHandler::getDefaultAllowed() const
{
	std::set<ArtifactID> allowedArtifacts;

	for (const auto & artifact : objects)
	{
		if (!artifact->isCombined())
			allowedArtifacts.insert(artifact->getId());
	}
	return allowedArtifacts;
}

void CArtHandler::afterLoadFinalization()
{
	//All artifacts have their id, so we can properly update their bonuses' source ids.
	for(auto &art : objects)
	{
		for(auto &bonus : art->getExportedBonusList())
		{
			assert(bonus->source == BonusSource::ARTIFACT);
			bonus->sid = BonusSourceID(art->id);
		}
		art->nodeHasChanged();
	}
}


VCMI_LIB_NAMESPACE_END
