/*
 * CCreatureHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCreatureHandler.h"

#include "CBonusTypeHandler.h"
#include "ResourceSet.h"
#include "entities/faction/CFaction.h"
#include "entities/faction/CTownHandler.h"
#include "filesystem/Filesystem.h"
#include "GameLibrary.h"
#include "modding/LibraryLoadContext.h"
#include "IGameSettings.h"
#include "constants/StringConstants.h"
#include "bonuses/Limiters.h"
#include "bonuses/Updaters.h"
#include "bonuses/BonusParameters.h"
#include "battle/CombatValue.h"
#include "json/JsonBonus.h"
#include "json/JsonUtils.h"
#include "serializer/JsonDeserializer.h"
#include "texts/CGeneralTextHandler.h"
#include "texts/CLegacyConfigParser.h"
#include "mapObjectConstructors/AObjectTypeHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "modding/CModHandler.h"
#include "ExceptionsCommon.h"

#include <vstd/RNG.h>

const std::map<CCreature::CreatureQuantityId, std::string> CCreature::creatureQuantityRanges =
{
		{CCreature::CreatureQuantityId::FEW, "1-4"},
		{CCreature::CreatureQuantityId::SEVERAL, "5-9"},
		{CCreature::CreatureQuantityId::PACK, "10-19"},
		{CCreature::CreatureQuantityId::LOTS, "20-49"},
		{CCreature::CreatureQuantityId::HORDE, "50-99"},
		{CCreature::CreatureQuantityId::THRONG, "100-249"},
		{CCreature::CreatureQuantityId::SWARM, "250-499"},
		{CCreature::CreatureQuantityId::ZOUNDS, "500-999"},
		{CCreature::CreatureQuantityId::LEGION, "1000+"}
};

int32_t CCreature::getIndex() const
{
	return idNumber.toEnum();
}

int32_t CCreature::getIconIndex() const
{
	return iconIndex;
}

std::string CCreature::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CCreature::getModScope() const
{
	return modScope;
}

void CCreature::registerIcons(const IconRegistar & cb) const
{
	cb(getIconIndex(), 0, "CPRSMALL", smallIconName);
	cb(getIconIndex(), 0, "TWCRPORT", largeIconName);
}

CreatureID CCreature::getId() const
{
	return idNumber;
}

const IBonusBearer * CCreature::getBonusBearer() const
{
	return this;
}

int32_t CCreature::getAdvMapAmountMin() const
{
	return ammMin;
}

int32_t CCreature::getAdvMapAmountMax() const
{
	return ammMax;
}

int32_t CCreature::getAIValue() const
{
	return AIValue;
}

int32_t CCreature::getFightValue() const
{
	return fightValue;
}

int32_t CCreature::getLevel() const
{
	return level;
}

int32_t CCreature::getGrowth() const
{
	return growth;
}

int32_t CCreature::getHorde() const
{
	return hordeGrowth;
}

FactionID CCreature::getFactionID() const
{
	return FactionID(faction);
}

int32_t CCreature::getBaseAttack() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK)).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseDefense() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE)).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseDamageMin() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMin).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseDamageMax() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMax).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseHitPoints() const
{
	static const auto SELECTOR = Selector::type()(BonusType::STACK_HEALTH).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseSpellPoints() const
{
	static const auto SELECTOR = Selector::type()(BonusType::CASTS).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseSpeed() const
{
	static const auto SELECTOR = Selector::type()(BonusType::STACKS_SPEED).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseShots() const
{
	static const auto SELECTOR = Selector::type()(BonusType::SHOTS).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getRecruitCost(GameResID resIndex) const
{
	return cost[resIndex];
}

const TResources & CCreature::getFullRecruitCost() const
{
	return cost;
}

bool CCreature::hasUpgrades() const
{
	return !upgrades.empty();
}

std::string CCreature::getNameTranslated() const
{
	return getNameSingularTranslated();
}

std::string CCreature::getNamePluralTranslated() const
{
	return LIBRARY->generaltexth->translate(getNamePluralTextID());
}

std::string CCreature::getNameSingularTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameSingularTextID());
}

std::string CCreature::getNameTextID() const
{
	return getNameSingularTextID();
}

std::string CCreature::getDescriptionTranslated() const
{
	return LIBRARY->generaltexth->translate(getDescriptionTextID());
}

std::string CCreature::getNamePluralTextID() const
{
	return TextIdentifier("creatures", modScope, identifier, "name", "plural" ).get();
}

std::string CCreature::getNameSingularTextID() const
{
	return TextIdentifier("creatures", modScope, identifier, "name", "singular" ).get();
}

std::string CCreature::getDescriptionTextID() const
{
	return TextIdentifier("creatures", modScope, identifier, "description").get();
}

std::string CCreature::getBonusTextID(const std::string & bonusID) const
{
	return TextIdentifier("creatures", modScope, identifier, "bonus", bonusID).get();
}

CCreature::CreatureQuantityId CCreature::getQuantityID(const int & quantity)
{
	if (quantity<5)
		return CCreature::CreatureQuantityId::FEW;
	if (quantity<10)
		return CCreature::CreatureQuantityId::SEVERAL;
	if (quantity<20)
		return CCreature::CreatureQuantityId::PACK;
	if (quantity<50)
		return CCreature::CreatureQuantityId::LOTS;
	if (quantity<100)
		return CCreature::CreatureQuantityId::HORDE;
	if (quantity<250)
		return CCreature::CreatureQuantityId::THRONG;
	if (quantity<500)
		return CCreature::CreatureQuantityId::SWARM;
	if (quantity<1000)
		return CCreature::CreatureQuantityId::ZOUNDS;

	return CCreature::CreatureQuantityId::LEGION;
}

std::string CCreature::getQuantityRangeStringForId(const CCreature::CreatureQuantityId & quantityId)
{
	if(creatureQuantityRanges.find(quantityId) != creatureQuantityRanges.end())
		return creatureQuantityRanges.at(quantityId);

	logGlobal->error("Wrong quantityId: %d", (int)quantityId);
	assert(0);
	return "[ERROR]";
}

int CCreature::estimateCreatureCount(ui32 countID)
{
	static const int creature_count[] = { 0, 3, 8, 15, 35, 75, 175, 375, 750, 2500 };

	if(countID > 9)
	{
		logGlobal->error("Wrong countID %d!", countID);
		return 0;
	}
	else
		return creature_count[countID];
}

bool CCreature::isDoubleWide() const
{
	return doubleWide;
}

/**
 * Determines if the creature is of a good alignment.
 * @return true if the creture is good, false otherwise.
 */
bool CCreature::isGood () const
{
	return LIBRARY->factions()->getById(faction)->getAlignment() == EAlignment::GOOD;
}
EAlignment CCreature::getAlignment () const
{
	return LIBRARY->factions()->getById(faction)->getAlignment();
}

/**
 * Determines if the creature is of an evil alignment.
 * @return true if the creature is evil, false otherwise.
 */
bool CCreature::isEvil () const
{
	return LIBRARY->factions()->getById(faction)->getAlignment() == EAlignment::EVIL;
}

si32 CCreature::maxAmount(const TResources &res) const //how many creatures can be bought
{
	int ret = 2147483645;
	for(const auto & [resource, amount] : cost.nonZeroEntries())
		ret = std::min(ret, (res[resource] / amount));
	return ret;
}

CCreature::CCreature()
	:CBonusSystemNode(BonusNodeType::CREATURE)
{
	fightValue = AIValue = growth = hordeGrowth = ammMin = ammMax = 0;
}

void CCreature::addBonus(int val, BonusType type)
{
	addBonus(val, type, BonusSubtypeID());
}

void CCreature::addBonus(int val, BonusType type, BonusSubtypeID subtype)
{
	auto selector = Selector::typeSubtype(type, subtype).And(Selector::source(BonusSource::CREATURE_ABILITY, BonusSourceID(getId())));
	BonusList & exported = getExportedBonusList();

	BonusList existing;
	exported.getBonuses(existing, selector);

	if(existing.empty())
	{
		auto added = std::make_shared<Bonus>(BonusDuration::PERMANENT, type, BonusSource::CREATURE_ABILITY, val, BonusSourceID(getId()), subtype, BonusValueType::BASE_NUMBER);
		addNewBonus(added);
	}
	else
	{
		std::shared_ptr<Bonus> b = existing[0];
		b->val = val;
	}
}

bool CCreature::isMyDirectUpgrade(const CCreature *anotherCre) const
{
	return vstd::contains(upgrades, anotherCre->getId());
}

bool CCreature::isMyDirectOrIndirectUpgrade(const CCreature *anotherCre) const
{
	std::set<CreatureID> foundUpgrades;
	std::vector<CreatureID> upgradesToTest;

	upgradesToTest.push_back(getId());

	while (!upgradesToTest.empty())
	{
		CreatureID testedID = upgradesToTest.back();
		const CCreature * testedPtr = testedID.toCreature();

		upgradesToTest.pop_back();

		for (const auto & upgrade : testedPtr->upgrades)
		{
			if (upgrade == anotherCre->getId())
				return true;

			if (foundUpgrades.count(upgrade))
				continue;

			upgradesToTest.push_back(upgrade);
			foundUpgrades.insert(upgrade);
		}
	}
	return false;
}

std::string CCreature::nodeName() const
{
	return "\"" + getNamePluralTextID() + "\"";
}

int CCreature::getRandomAmount(vstd::RNG & ranGen) const
{
	if(ammMax > ammMin)
		return ammMin + (ranGen.nextInt(ammMin, ammMax));
	else
		return ammMax;
}

void CCreature::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("fightValue", fightValue);
	handler.serializeInt("aiValue", AIValue);
	handler.serializeInt("growth", growth);
	handler.serializeInt("horde", hordeGrowth);// Needed at least until configurable buildings

	{
		auto advMapNode = handler.enterStruct("advMapAmount");
		handler.serializeInt("min", ammMin);
		handler.serializeInt("max", ammMax);
	}

	handler.serializeInt("level", level);
	handler.serializeBool("doubleWide", doubleWide);

	if(!handler.saving)
	{
		if(ammMin > ammMax)
		{
			logMod->error("Invalid creature '%s' configuration, advMapAmount.min > advMapAmount.max", identifier);
			std::swap(ammMin, ammMax);
		}
	}
}

CCreatureHandler::CCreatureHandler(LibraryLoadContext & context)
	: expAfterUpgrade(0)
{
	loadCommanders(context);
}

void CCreatureHandler::loadCommanders(LibraryLoadContext & context)
{
	const auto & parseBonusWithCompatibility = [&context](const JsonNode & node)
	{
		// MOD COMPATIBILITY: 1.6 mods use old, vector format.
		// NOTE: please also remove parseBonus itself - commanders is the last place that uses it
		if (node.isVector())
			return JsonUtils::parseBonus(node.Vector(), context);
		else
			return JsonUtils::parseBonus(node, context);
	};

	auto configResource = JsonPath::builtin("config/commanders.json");

	std::string modSource = context.lib.modh->findResourceOrigin(configResource);
	JsonNode data(configResource);
	data.setModScope(modSource);

	const JsonNode & config = data; // switch to const data accessors

	commanderResurrectionPrice.resolveFromJson(config["resurrectionPrice"], context.resolver);

	for (auto bonus : config["bonusPerLevel"].Vector())
	{
		commanderLevelPremy.push_back(parseBonusWithCompatibility(bonus));
	}

	int level = 0;
	for (auto skill : config["skillLevels"].Vector())
	{
		skillLevels.emplace_back();
		for (auto skillLevel : skill["levels"].Vector())
		{
			skillLevels[level].push_back(static_cast<ui8>(skillLevel.Float()));
		}
		++level;
	}

	for (const auto & abilityRequirements : config["abilityRequirements"].Vector())
	{
		std::pair <std::vector<std::shared_ptr<Bonus> >, std::pair <ui8, ui8> > a;
		const JsonNode & abilities = abilityRequirements["ability"];

		if (abilities[0].isString()) // old format with single bonus
		{
			a.first.push_back(parseBonusWithCompatibility(abilities));
		}
		else
		{
			for (const auto & ability : abilities.Vector())
				a.first.push_back(parseBonusWithCompatibility(ability));
		}

		a.second.first =  static_cast<ui8>(abilityRequirements["skills"][0].Float());
		a.second.second = static_cast<ui8>(abilityRequirements["skills"][1].Float());
		skillRequirements.push_back (a);
	}
}

std::vector<JsonNode> CCreatureHandler::loadLegacyData(LibraryLoadContext & context)
{
	size_t dataSize = context.lib.engineSettings()->getInteger(EGameSettings::TEXTS_CREATURE);

	objects.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser parser(TextPath::builtin("DATA/CRTRAITS.TXT"), *context.lib.modh);

	parser.endLine(); // header

	// this file is a bit different in some of Russian localisations:
	//ENG: Singular	Plural Wood ...
	//RUS: Singular	Plural	Plural2 Wood ...
	// Try to detect which version this is by header
	// TODO: use 3rd name? Stand for "whose", e.g. pikemans'
	size_t namesCount = 2;
	{
		if ( parser.readString() != "Singular" || parser.readString() != "Plural" )
			throw std::runtime_error("Incorrect format of CrTraits.txt");

		if (parser.readString() == "Plural2")
			namesCount = 3;

		parser.endLine();
	}

	const bool isRoe = context.lib.isRoeData();

	for (size_t i=0; i<dataSize; i++)
	{
		//loop till non-empty line
		while (parser.isNextEntryEmpty())
			if(!parser.endLine())
				break;

		JsonNode data;

		data["name"]["singular"].String() =  parser.readString();

		if (namesCount == 3)
			parser.readString();

		data["name"]["plural"].String() =  parser.readString();

		for(int v=0; v<GameConstants::RESOURCE_QUANTITY; ++v)
			data["cost"][GameConstants::RESOURCE_NAMES[v]].Float() = parser.readNumber();

		data["fightValue"].Float() = parser.readNumber();
		data["aiValue"].Float() = parser.readNumber();
		data["growth"].Float() = parser.readNumber();
		data["horde"].Float() = parser.readNumber();

		data["hitPoints"].Float() = parser.readNumber();
		data["speed"].Float() = parser.readNumber();
		data["attack"].Float() = parser.readNumber();
		data["defense"].Float() = parser.readNumber();
		data["damage"]["min"].Float() = parser.readNumber();
		data["damage"]["max"].Float() = parser.readNumber();

		if (float shots = parser.readNumber())
			data["shots"].Float() = shots;

		if(!isRoe)
			if (float spells = parser.readNumber())
				data["spellPoints"].Float() = spells;

		data["advMapAmount"]["min"].Float() = parser.readNumber();
		data["advMapAmount"]["max"].Float() = parser.readNumber();

		// unused - ability text, not used since we no longer have original creature window
		parser.readString();
		parser.readString(); // unused - abilities, not used since we load them all from json configs

		h3Data.push_back(data);
	}

	loadAnimationInfo(context, h3Data);

	return h3Data;
}

std::shared_ptr<CCreature> CCreatureHandler::loadFromJson(LibraryLoadContext & context, const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	auto cre = std::make_shared<CCreature>();

	if(node["hasDoubleWeek"].Bool())
	{
		doubledCreatures.insert(CreatureID(index));
	}
	cre->idNumber = CreatureID(index);
	cre->iconIndex = cre->getIndex() + 2;
	cre->identifier = identifier;
	cre->modScope = scope;

	JsonDeserializer handler(nullptr, context.resolver, node);
	cre->serializeJson(handler);

	cre->cost.resolveFromJson(node["cost"], context.resolver);
	if(!node["resurrectionPrice"].isNull())
	{
		cre->commanderResurrectionPrice.emplace();
		cre->commanderResurrectionPrice->resolveFromJson(node["resurrectionPrice"], context.resolver);
	}

	context.lib.generaltexth->registerString(scope, cre->getNameSingularTextID(), node["name"]["singular"]);
	context.lib.generaltexth->registerString(scope, cre->getNamePluralTextID(), node["name"]["plural"]);
	context.lib.generaltexth->registerString(scope, cre->getDescriptionTextID(), node["description"]);

	cre->addBonus(node["hitPoints"].Integer(), BonusType::STACK_HEALTH);
	cre->addBonus(node["speed"].Integer(), BonusType::STACKS_SPEED);
	cre->addBonus(node["attack"].Integer(), BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK));
	cre->addBonus(node["defense"].Integer(), BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE));

	int minDamage = node["damage"]["min"].Integer();
	int maxDamage = node["damage"]["max"].Integer();

	if (minDamage <= maxDamage)
	{
		cre->addBonus(minDamage, BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMin);
		cre->addBonus(maxDamage, BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMax);
	}
	else
	{
		logMod->error("Mod %s: creature %s has minimal damage (%d) greater than maximal damage (%d)!", scope, identifier, minDamage, maxDamage);
		cre->addBonus(maxDamage, BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMin);
		cre->addBonus(minDamage, BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMax);
	}

	if(!node["shots"].isNull())
		cre->addBonus(node["shots"].Integer(), BonusType::SHOTS);

	loadStackExperience(context, cre.get(), node["stackExperience"]);
	loadJsonAnimation(cre.get(), node["graphics"]);
	loadCreatureJson(context, cre.get(), node);

	// towns and dwellings produce and guard units in amounts based on growth
	if (cre->getGrowth() < 0 || (cre->getGrowth() == 0 && !cre->special))
	{
		const int fixedGrowth = cre->special ? 0 : 1;
		logMod->error("Mod %s: creature %s has invalid growth %d, using %d instead", scope, identifier, cre->getGrowth(), fixedGrowth);
		cre->growth = fixedGrowth;
	}

	if (!node["disabled"].Bool() &&
		!CResourceHandler::get()->existsResource(cre->animDefName) &&
		!CResourceHandler::get()->existsResource(cre->animDefName.toType<EResType::JSON>()) &&
		!CResourceHandler::get()->existsResource(cre->animDefName.addPrefix("SPRITES/")) &&
		!CResourceHandler::get()->existsResource(cre->animDefName.addPrefix("SPRITES/").toType<EResType::JSON>()))
		throw ModLoadingException(scope, "creature " + cre->getJsonKey() + " has no combat animation but is not marked as special!" );

	JsonNode advMapFile = node["graphics"]["map"];
	JsonNode advMapMask = node["graphics"]["mapMask"];

	context.resolver.requestIdentifier(scope, "object", "monster", [&context, cre, scope, advMapFile, advMapMask](si32 monsterIndex)
	{
		JsonNode conf;
		conf.setModScope(scope);

		context.lib.objtypeh->loadSubObject(context, cre->identifier, conf, Obj::MONSTER, cre->getId().num);
		if (!advMapFile.isNull())
		{
			JsonNode templ;
			templ["animation"] = advMapFile;
			if (!advMapMask.isNull())
				templ["mask"] = advMapMask;
			templ.setModScope(scope);

			// if creature has custom advMapFile, reset any potentially imported H3M templates and use provided file instead
			context.lib.objtypeh->getHandlerFor(Obj::MONSTER, cre->getId().num)->clearTemplates();
			context.lib.objtypeh->getHandlerFor(Obj::MONSTER, cre->getId().num)->addTemplate(context, templ);
		}

		// object does not have any templates - this is not usable object (e.g. pseudo-creature like Arrow Tower)
		if (context.lib.objtypeh->getHandlerFor(Obj::MONSTER, cre->getId().num)->getTemplates().empty())
		{
			if (!cre->special)
				throw ModLoadingException(scope, "creature " + cre->getJsonKey() + " has no adventure map animation but is not marked as special!" );

			context.lib.objtypeh->removeSubObject(Obj::MONSTER, cre->getId().num);
		}
	});

	return cre;
}

const std::vector<std::string> & CCreatureHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "creature" };
	return typeNames;
}

void CCreatureHandler::loadCrExpMod(LibraryLoadContext & context)
{
	if (context.lib.engineSettings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE)) 	//reading default stack experience values
	{
		//Calculate rank exp values, formula appears complicated bu no parsing needed
		expRanks.resize(8);
		int dif = 0;
		int it = 8000; //ignore name of this variable
		expRanks[0].push_back(it);
		for (int j = 1; j < 10; ++j) //used for tiers 8-10, and all other probably
		{
			expRanks[0].push_back(expRanks[0][j-1] + it + dif);
			dif += it/5;
		}
		for (int i = 1; i < 8; ++i) //used for tiers 1-7
		{
			dif = 0;
			it = 1000 * i;
			expRanks[i].push_back(it);
			for (int j = 1; j < 10; ++j)
			{
				expRanks[i].push_back(expRanks[i][j-1] + it + dif);
				dif += it/5;
			}
		}

		CLegacyConfigParser expBonParser(TextPath::builtin("DATA/CREXPMOD.TXT"), *context.lib.modh);

		expBonParser.endLine(); //header

		maxExpPerBattle.resize(8);
		for (int i = 1; i < 8; ++i)
		{
			expBonParser.readString(); //index
			expBonParser.readString(); //float multiplier -> hardcoded
			expBonParser.readString(); //ignore upgrade mod? ->hardcoded
			expBonParser.readString(); //already calculated

			maxExpPerBattle[i] = static_cast<ui32>(expBonParser.readNumber());
			expRanks[i].push_back(expRanks[i].back() + static_cast<ui32>(expBonParser.readNumber()));

			expBonParser.endLine();
		}
		//exp for tier >7, rank 11
		expRanks[0].push_back(147000);
		expAfterUpgrade = 75; //percent
		maxExpPerBattle[0] = maxExpPerBattle[7];
	}
}

void CCreatureHandler::loadAnimationInfo(LibraryLoadContext & context, std::vector<JsonNode> &h3Data) const
{
	CLegacyConfigParser parser(TextPath::builtin("DATA/CRANIM.TXT"), *context.lib.modh);

	parser.endLine(); // header
	parser.endLine();

	for(int dd = 0; dd < context.lib.engineSettings()->getInteger(EGameSettings::TEXTS_CREATURE); ++dd)
	{
		while (parser.isNextEntryEmpty() && parser.endLine()) // skip empty lines
			;

		loadUnitAnimInfo(h3Data[dd]["graphics"], parser);
		parser.endLine();
	}
}

void CCreatureHandler::loadUnitAnimInfo(JsonNode & graphics, CLegacyConfigParser & parser) const
{
	graphics["timeBetweenFidgets"].Float() = parser.readNumber();

	JsonNode & animationTime = graphics["animationTime"];
	animationTime["walk"].Float() = parser.readNumber();
	animationTime["attack"].Float() = parser.readNumber();
	parser.readNumber(); // unused value "Flight animation time" - H3 actually uses "Walk animation time" even for flying creatures
	animationTime["idle"].Float() = 10.0;

	JsonNode & missile = graphics["missile"];
	JsonNode & offsets = missile["offset"];

	offsets["upperX"].Float() = parser.readNumber();
	offsets["upperY"].Float() = parser.readNumber();
	offsets["middleX"].Float() = parser.readNumber();
	offsets["middleY"].Float() = parser.readNumber();
	offsets["lowerX"].Float() = parser.readNumber();
	offsets["lowerY"].Float() = parser.readNumber();

	for(int i=0; i<12; i++)
	{
		JsonNode entry;
		entry.Float() = parser.readNumber();
		missile["frameAngles"].Vector().push_back(entry);
	}

	// Unused property "troopCountLocationOffset"
	parser.readNumber();

	missile["attackClimaxFrame"].Float() = parser.readNumber();

	// assume that creature is not a shooter and should not have whole missile field
	if (missile["frameAngles"].Vector()[0].Integer() == 0 &&
		missile["attackClimaxFrame"].Integer() == 0)
		graphics.Struct().erase("missile");
}

RayColor RayColor::fromJson(const JsonNode & node)
{
	RayColor color;
	color.start.r = node["start"].Vector()[0].Integer();
	color.start.g = node["start"].Vector()[1].Integer();
	color.start.b = node["start"].Vector()[2].Integer();
	color.start.a = node["start"].Vector()[3].Integer();

	color.end.r = node["end"].Vector()[0].Integer();
	color.end.g = node["end"].Vector()[1].Integer();
	color.end.b = node["end"].Vector()[2].Integer();
	color.end.a = node["end"].Vector()[3].Integer();
	return color;
}

void CCreatureHandler::loadJsonAnimation(CCreature * cre, const JsonNode & graphics) const
{
	cre->animation.timeBetweenFidgets = graphics["timeBetweenFidgets"].Float();

	const JsonNode & animationTime = graphics["animationTime"];
	cre->animation.walkAnimationTime = animationTime["walk"].Float();
	cre->animation.idleAnimationTime = animationTime["idle"].Float();
	cre->animation.attackAnimationTime = animationTime["attack"].Float();

	const JsonNode & missile = graphics["missile"];
	const JsonNode & offsets = missile["offset"];
	cre->animation.upperRightMissileOffsetX = static_cast<int>(offsets["upperX"].Float());
	cre->animation.upperRightMissileOffsetY = static_cast<int>(offsets["upperY"].Float());
	cre->animation.rightMissileOffsetX =      static_cast<int>(offsets["middleX"].Float());
	cre->animation.rightMissileOffsetY =      static_cast<int>(offsets["middleY"].Float());
	cre->animation.lowerRightMissileOffsetX = static_cast<int>(offsets["lowerX"].Float());
	cre->animation.lowerRightMissileOffsetY = static_cast<int>(offsets["lowerY"].Float());

	cre->animation.attackClimaxFrame = static_cast<int>(missile["attackClimaxFrame"].Float());
	cre->animation.missileFrameAngles = missile["frameAngles"].convertTo<std::vector<double> >();

	cre->smallIconName = graphics["iconSmall"].String();
	cre->largeIconName = graphics["iconLarge"].String();
}

void CCreatureHandler::loadCreatureJson(LibraryLoadContext & context, CCreature * creature, const JsonNode & config) const
{
	creature->animDefName = AnimationPath::fromJson(config["graphics"]["animation"]);
	creature->mapAttackFromLeft = ImagePath::fromJson(config["graphics"]["mapAttackFromLeft"]);
	creature->mapAttackFromRight = ImagePath::fromJson(config["graphics"]["mapAttackFromRight"]);

	for(const auto & ability : config["abilities"].Struct())
	{
		if (!ability.second.isNull())
		{
			auto b = JsonUtils::parseBonus(ability.second, context, creature->getBonusTextID(ability.first));
			b->source = BonusSource::CREATURE_ABILITY;
			b->sid = BonusSourceID(creature->getId());
			b->duration = BonusDuration::PERMANENT;
			creature->addNewBonus(b);
		}
	}

	context.resolver.requestIdentifier("faction", config["faction"], [=](si32 faction)
	{
		creature->faction = FactionID(faction);
	});

	for(const JsonNode &value : config["upgrades"].Vector())
	{
		context.resolver.requestIdentifier("creature", value, [=](si32 identifier)
		{
			creature->upgrades.insert(CreatureID(identifier));
		});
	}

	creature->animation.projectileImageName = AnimationPath::fromJson(config["graphics"]["missile"]["projectile"]);

	for(const JsonNode & value : config["graphics"]["missile"]["ray"].Vector())
		creature->animation.projectileRay.push_back(RayColor::fromJson(value));

	creature->special = config["special"].Bool() || config["disabled"].Bool();
	creature->excludeFromRandomization = config["excludeFromRandomization"].Bool();

	const JsonNode & sounds = config["sound"];
	creature->sounds.attack = AudioPath::fromJson(sounds["attack"]);
	creature->sounds.defend = AudioPath::fromJson(sounds["defend"]);
	creature->sounds.killed = AudioPath::fromJson(sounds["killed"]);
	creature->sounds.move = AudioPath::fromJson(sounds["move"]);
	creature->sounds.shoot = AudioPath::fromJson(sounds["shoot"]);
	creature->sounds.cast = AudioPath::fromJson(sounds["cast"]);
	creature->sounds.wince = AudioPath::fromJson(sounds["wince"]);
	creature->sounds.startMoving = AudioPath::fromJson(sounds["startMoving"]);
	creature->sounds.endMoving = AudioPath::fromJson(sounds["endMoving"]);
}

namespace
{

/// Ranks of stack experience that share the same value of a bonus
struct StackExperienceRange
{
	int value;
	int firstRank;
	int lastRank;
	bool openEnded; ///< value is also kept by all ranks above the last one

	std::shared_ptr<RankRangeLimiter> limiter() const
	{
		if (openEnded)
			return std::make_shared<RankRangeLimiter>(firstRank);
		return std::make_shared<RankRangeLimiter>(firstRank, lastRank + 1);
	}
};

/// Splits values of a bonus for ranks 1, 2, ... into ranges of ranks with the same value, so that a stack
/// of any rank receives a single bonus with the total value for its rank. Ranks with no bonus are skipped.
std::vector<StackExperienceRange> stackExperienceRanges(const std::vector<int> & rankValues)
{
	std::vector<StackExperienceRange> result;
	size_t rangeStart = 0;
	for (size_t i = 1; i <= rankValues.size(); ++i)
	{
		if (i < rankValues.size() && rankValues[i] == rankValues[rangeStart])
			continue;

		if (rankValues[rangeStart] != 0)
			result.push_back({rankValues[rangeStart], static_cast<int>(rangeStart) + 1, static_cast<int>(i), i == rankValues.size()});

		rangeStart = i;
	}
	return result;
}

}

void CCreatureHandler::loadStackExperience(LibraryLoadContext & context, CCreature * creature, const JsonNode & input) const
{
	for (size_t entryIndex = 0; entryIndex < input.Vector().size(); ++entryIndex)
	{
		const JsonNode & exp = input.Vector()[entryIndex];
		const std::string descriptionID = creature->getBonusTextID("stackExperience" + std::to_string(entryIndex));
		const JsonVector &values = exp["values"].Vector();
		// values[0] is the value for rank 1
		int lowerLimit = 1;
		if (values[0].getType() == JsonNode::JsonType::DATA_BOOL)
		{
			for (const JsonNode &val : values)
			{
				if(val.Bool())
				{
					// parse each bonus separately
					// we can not create copies since identifiers resolution does not tracks copies
					// leading to unset identifier values in copies
					auto bonus = JsonUtils::parseBonus(exp["bonus"], context, descriptionID);
					bonus->source = BonusSource::STACK_EXPERIENCE;
					bonus->duration = BonusDuration::PERMANENT;
					bonus->addLimiter(std::make_shared<RankRangeLimiter>(lowerLimit));
					creature->addNewBonus (bonus);
					break; //TODO: allow bonuses to turn off?
				}
				++lowerLimit;
			}
		}
		else
		{
			std::vector<int> rankValues;
			for (const JsonNode &val : values)
				rankValues.push_back(static_cast<int>(val.Integer()));

			for (const auto & range : stackExperienceRanges(rankValues))
			{
				JsonNode bonusInput = exp["bonus"];
				bonusInput["val"].Float() = range.value;

				auto bonus = JsonUtils::parseBonus(bonusInput, context, descriptionID);
				bonus->source = BonusSource::STACK_EXPERIENCE;
				bonus->duration = BonusDuration::PERMANENT;
				bonus->addLimiter(range.limiter());
				creature->addNewBonus (bonus);
			}
		}
	}
}

CCreatureHandler::~CCreatureHandler()
{
	for(auto & p : skillRequirements)
		p.first.clear();
}

void CCreatureHandler::buildCombatValues(const GameLibrary & library)
{
	combatValues = std::make_unique<CombatValue>(library);
}

const CombatValue & CCreatureHandler::getCombatValue() const
{
	assert(combatValues);
	return *combatValues;
}

const ResourceSet & CCreatureHandler::getCommanderResurrectionPrice(const CCreature * commander) const
{
	if(commander && commander->commanderResurrectionPrice)
		return *commander->commanderResurrectionPrice;

	return commanderResurrectionPrice;
}

void CCreatureHandler::afterLoadFinalization(LibraryLoadContext & context)
{
	for(auto & creature : objects)
	{
		if (!creature)
			continue;

		auto natureBonuses = creature->getBonuses([&context](const Bonus * b){
				return context.lib.bth->isCreatureNatureBonus(b->type);
		});

		if (natureBonuses->empty())
			creature->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LIVING, BonusSource::CREATURE_ABILITY, 0, BonusSourceID(creature->getId())));
	}
}

std::set<CreatureID> CCreatureHandler::getDefaultAllowed() const
{
	std::set<CreatureID> result;

	for(auto & creature : objects)
		if (creature && !creature->special)
			result.insert(creature->getId());

	return result;
}
