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

#include "CGeneralTextHandler.h"
#include "ResourceSet.h"
#include "filesystem/Filesystem.h"
#include "VCMI_Lib.h"
#include "CTownHandler.h"
#include "GameSettings.h"
#include "StringConstants.h"
#include "bonuses/Limiters.h"
#include "bonuses/Updaters.h"
#include "serializer/JsonDeserializer.h"
#include "serializer/JsonUpdater.h"
#include "mapObjectConstructors/AObjectTypeHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "modding/CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

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

FactionID CCreature::getFaction() const
{
	return FactionID(faction);
}

int32_t CCreature::getBaseAttack() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::PRIMARY_SKILL, PrimarySkill::ATTACK).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseDefense() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::PRIMARY_SKILL, PrimarySkill::DEFENSE).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseDamageMin() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, 1).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
	return getExportedBonusList().valOfBonuses(SELECTOR);
}

int32_t CCreature::getBaseDamageMax() const
{
	static const auto SELECTOR = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, 2).And(Selector::sourceTypeSel(BonusSource::CREATURE_ABILITY));
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
	if(resIndex >= 0 && resIndex < cost.size())
		return cost[resIndex];
	else
		return 0;
}

TResources CCreature::getFullRecruitCost() const
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
	return VLC->generaltexth->translate(getNamePluralTextID());
}

std::string CCreature::getNameSingularTranslated() const
{
	return VLC->generaltexth->translate(getNameSingularTextID());
}

std::string CCreature::getNameTextID() const
{
	return getNameSingularTextID();
}

std::string CCreature::getNamePluralTextID() const
{
	return TextIdentifier("creatures", modScope, identifier, "name", "plural" ).get();
}

std::string CCreature::getNameSingularTextID() const
{
	return TextIdentifier("creatures", modScope, identifier, "name", "singular" ).get();
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
	return VLC->factions()->getByIndex(faction)->getAlignment() == EAlignment::GOOD;
}

/**
 * Determines if the creature is of an evil alignment.
 * @return true if the creature is evil, false otherwise.
 */
bool CCreature::isEvil () const
{
	return VLC->factions()->getByIndex(faction)->getAlignment() == EAlignment::EVIL;
}

si32 CCreature::maxAmount(const TResources &res) const //how many creatures can be bought
{
	int ret = 2147483645;
	int resAmnt = static_cast<int>(std::min(res.size(),cost.size()));
	for(int i=0;i<resAmnt;i++)
		if(cost[i])
			ret = std::min(ret, (res[i] / cost[i]));
	return ret;
}

CCreature::CCreature()
{
	setNodeType(CBonusSystemNode::CREATURE);
	fightValue = AIValue = growth = hordeGrowth = ammMin = ammMax = 0;
}

void CCreature::addBonus(int val, BonusType type, int subtype)
{
	auto selector = Selector::typeSubtype(type, subtype).And(Selector::source(BonusSource::CREATURE_ABILITY, getIndex()));
	BonusList & exported = getExportedBonusList();

	BonusList existing;
	exported.getBonuses(existing, selector, Selector::all);

	if(existing.empty())
	{
		auto added = std::make_shared<Bonus>(BonusDuration::PERMANENT, type, BonusSource::CREATURE_ABILITY, val, getIndex(), subtype, BonusValueType::BASE_NUMBER);
		addNewBonus(added);
	}
	else
	{
		std::shared_ptr<Bonus> b = existing[0];
		b->val = val;
	}
}

bool CCreature::isMyUpgrade(const CCreature *anotherCre) const
{
	//TODO upgrade of upgrade?
	return vstd::contains(upgrades, anotherCre->getId());
}

bool CCreature::valid() const
{
	return this == VLC->creh->objects[idNumber];
}

std::string CCreature::nodeName() const
{
	return "\"" + getNamePluralTextID() + "\"";
}

void CCreature::updateFrom(const JsonNode & data)
{
	JsonUpdater handler(nullptr, data);

	{
		auto configScope = handler.enterStruct("config");

		const JsonNode & configNode = handler.getCurrent();

		serializeJson(handler);

		if(!configNode["hitPoints"].isNull())
			addBonus(configNode["hitPoints"].Integer(), BonusType::STACK_HEALTH);

		if(!configNode["speed"].isNull())
			addBonus(configNode["speed"].Integer(), BonusType::STACKS_SPEED);

		if(!configNode["attack"].isNull())
			addBonus(configNode["attack"].Integer(), BonusType::PRIMARY_SKILL, PrimarySkill::ATTACK);

		if(!configNode["defense"].isNull())
			addBonus(configNode["defense"].Integer(), BonusType::PRIMARY_SKILL, PrimarySkill::DEFENSE);

		if(!configNode["damage"]["min"].isNull())
			addBonus(configNode["damage"]["min"].Integer(), BonusType::CREATURE_DAMAGE, 1);

		if(!configNode["damage"]["max"].isNull())
			addBonus(configNode["damage"]["max"].Integer(), BonusType::CREATURE_DAMAGE, 2);

		if(!configNode["shots"].isNull())
			addBonus(configNode["shots"].Integer(), BonusType::SHOTS);

		if(!configNode["spellPoints"].isNull())
			addBonus(configNode["spellPoints"].Integer(), BonusType::CASTS);
	}


	handler.serializeBonuses("bonuses", this);
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

	if(handler.updating)
	{
		cost.serializeJson(handler, "cost");
		handler.serializeInt("faction", faction);//TODO: unify with deferred resolve
	}

	handler.serializeInt("level", level);
	handler.serializeBool("doubleWide", doubleWide);

	if(!handler.saving)
	{
		if(ammMin > ammMax)
			logMod->error("Invalid creature '%s' configuration, advMapAmount.min > advMapAmount.max", identifier);
	}
}

CCreatureHandler::CCreatureHandler()
	: expAfterUpgrade(0)
{
	VLC->creh = this;
	loadCommanders();
}

const CCreature * CCreatureHandler::getCreature(const std::string & scope, const std::string & identifier) const
{
	std::optional<si32> index = VLC->identifiers()->getIdentifier(scope, "creature", identifier);

	if(!index)
		throw std::runtime_error("Creature not found "+identifier);

	return objects[*index];
}

void CCreatureHandler::loadCommanders()
{
	ResourceID configResource("config/commanders.json");

	std::string modSource = VLC->modh->findResourceOrigin(configResource);
	JsonNode data(configResource);
	data.setMeta(modSource);

	const JsonNode & config = data; // switch to const data accessors

	for (auto bonus : config["bonusPerLevel"].Vector())
	{
		commanderLevelPremy.push_back(JsonUtils::parseBonus(bonus.Vector()));
	}

	int i = 0;
	for (auto skill : config["skillLevels"].Vector())
	{
		skillLevels.emplace_back();
		for (auto skillLevel : skill["levels"].Vector())
		{
			skillLevels[i].push_back(static_cast<ui8>(skillLevel.Float()));
		}
		++i;
	}

	for (auto ability : config["abilityRequirements"].Vector())
	{
		std::pair <std::shared_ptr<Bonus>, std::pair <ui8, ui8> > a;
		a.first = JsonUtils::parseBonus (ability["ability"].Vector());
		a.second.first =  static_cast<ui8>(ability["skills"].Vector()[0].Float());
		a.second.second = static_cast<ui8>(ability["skills"].Vector()[1].Float());
		skillRequirements.push_back (a);
	}
}

void CCreatureHandler::loadBonuses(JsonNode & creature, std::string bonuses) const
{
	auto makeBonusNode = [&](const std::string & type, double val = 0) -> JsonNode
	{
		JsonNode ret;
		ret["type"].String() = type;
		ret["val"].Float() = val;
		return ret;
	};

	static const std::map<std::string, JsonNode> abilityMap =
	{
		{"FLYING_ARMY",            makeBonusNode("FLYING")},
		{"SHOOTING_ARMY",          makeBonusNode("SHOOTER")},
		{"SIEGE_WEAPON",           makeBonusNode("SIEGE_WEAPON")},
		{"const_free_attack",      makeBonusNode("BLOCKS_RETALIATION")},
		{"IS_UNDEAD",              makeBonusNode("UNDEAD")},
		{"const_no_melee_penalty", makeBonusNode("NO_MELEE_PENALTY")},
		{"const_jousting",         makeBonusNode("JOUSTING", 5)},
		{"KING_1",                 makeBonusNode("KING")}, // Slayer with no expertise
		{"KING_2",                 makeBonusNode("KING", 2)}, // Advanced Slayer or better
		{"KING_3",                 makeBonusNode("KING", 3)}, // Expert Slayer only
		{"const_no_wall_penalty",  makeBonusNode("NO_WALL_PENALTY")},
		{"MULTI_HEADED",           makeBonusNode("ATTACKS_ALL_ADJACENT")},
		{"IMMUNE_TO_MIND_SPELLS",  makeBonusNode("MIND_IMMUNITY")},
		{"HAS_EXTENDED_ATTACK",    makeBonusNode("TWO_HEX_ATTACK_BREATH")}
	};

	auto hasAbility = [&](const std::string & name) -> bool 
	{
		return boost::algorithm::find_first(bonuses, name);
	};

	for(const auto & a : abilityMap)
	{
		if(hasAbility(a.first))
			creature["abilities"][a.first] = a.second;
	}
	if(hasAbility("DOUBLE_WIDE"))
		creature["doubleWide"].Bool() = true;

	if(hasAbility("const_raises_morale"))
	{
		JsonNode node = makeBonusNode("MORALE");
		node["val"].Float() = 1;
		node["propagator"].String() = "HERO";
		creature["abilities"]["const_raises_morale"] = node;
	}
}

std::vector<JsonNode> CCreatureHandler::loadLegacyData()
{
	size_t dataSize = VLC->settings()->getInteger(EGameSettings::TEXTS_CREATURE);

	objects.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser parser("DATA/CRTRAITS.TXT");

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

	for (size_t i=0; i<dataSize; i++)
	{
		//loop till non-empty line
		while (parser.isNextEntryEmpty())
			parser.endLine();

		JsonNode data;

		data["name"]["singular"].String() =  parser.readString();

		if (namesCount == 3)
			parser.readString();

		data["name"]["plural"].String() =  parser.readString();

		for(int v=0; v<7; ++v)
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

		if (float spells = parser.readNumber())
			data["spellPoints"].Float() = spells;

		data["advMapAmount"]["min"].Float() = parser.readNumber();
		data["advMapAmount"]["max"].Float() = parser.readNumber();

		data["abilityText"].String() = parser.readString();
		loadBonuses(data, parser.readString()); //Attributes

		h3Data.push_back(data);
	}

	loadAnimationInfo(h3Data);

	return h3Data;
}

CCreature * CCreatureHandler::loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	auto * cre = new CCreature();

	if(node["hasDoubleWeek"].Bool())
	{
		doubledCreatures.insert(CreatureID(index));
	}
	cre->idNumber = CreatureID(index);
	cre->iconIndex = cre->getIndex() + 2;
	cre->identifier = identifier;
	cre->modScope = scope;

	JsonDeserializer handler(nullptr, node);
	cre->serializeJson(handler);

	cre->cost = ResourceSet(node["cost"]);

	VLC->generaltexth->registerString(scope, cre->getNameSingularTextID(), node["name"]["singular"].String());
	VLC->generaltexth->registerString(scope, cre->getNamePluralTextID(), node["name"]["plural"].String());

	cre->addBonus(node["hitPoints"].Integer(), BonusType::STACK_HEALTH);
	cre->addBonus(node["speed"].Integer(), BonusType::STACKS_SPEED);
	cre->addBonus(node["attack"].Integer(), BonusType::PRIMARY_SKILL, PrimarySkill::ATTACK);
	cre->addBonus(node["defense"].Integer(), BonusType::PRIMARY_SKILL, PrimarySkill::DEFENSE);

	cre->addBonus(node["damage"]["min"].Integer(), BonusType::CREATURE_DAMAGE, 1);
	cre->addBonus(node["damage"]["max"].Integer(), BonusType::CREATURE_DAMAGE, 2);

	assert(node["damage"]["min"].Integer() <= node["damage"]["max"].Integer());

	if(!node["shots"].isNull())
		cre->addBonus(node["shots"].Integer(), BonusType::SHOTS);

	loadStackExperience(cre, node["stackExperience"]);
	loadJsonAnimation(cre, node["graphics"]);
	loadCreatureJson(cre, node);

	for(const auto & extraName : node["extraNames"].Vector())
	{
		for(const auto & type_name : getTypeNames())
			registerObject(scope, type_name, extraName.String(), cre->getIndex());
	}

	JsonNode advMapFile = node["graphics"]["map"];
	JsonNode advMapMask = node["graphics"]["mapMask"];

	VLC->identifiers()->requestIdentifier(scope, "object", "monster", [=](si32 index)
	{
		JsonNode conf;
		conf.setMeta(scope);

		VLC->objtypeh->loadSubObject(cre->identifier, conf, Obj::MONSTER, cre->getId().num);
		if (!advMapFile.isNull())
		{
			JsonNode templ;
			templ["animation"] = advMapFile;
			if (!advMapMask.isNull())
				templ["mask"] = advMapMask;
			templ.setMeta(scope);

			// if creature has custom advMapFile, reset any potentially imported H3M templates and use provided file instead
			VLC->objtypeh->getHandlerFor(Obj::MONSTER, cre->getId().num)->clearTemplates();
			VLC->objtypeh->getHandlerFor(Obj::MONSTER, cre->getId().num)->addTemplate(templ);
		}

		// object does not have any templates - this is not usable object (e.g. pseudo-creature like Arrow Tower)
		if (VLC->objtypeh->getHandlerFor(Obj::MONSTER, cre->getId().num)->getTemplates().empty())
			VLC->objtypeh->removeSubObject(Obj::MONSTER, cre->getId().num);
	});

	return cre;
}

const std::vector<std::string> & CCreatureHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "creature" };
	return typeNames;
}

std::vector<bool> CCreatureHandler::getDefaultAllowed() const
{
	std::vector<bool> ret;

	ret.reserve(objects.size());
	for(const CCreature * crea : objects)
	{
		ret.push_back(crea ? !crea->special : false);
	}
	return ret;
}

void CCreatureHandler::loadCrExpMod()
{
	if (VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE)) 	//reading default stack experience values
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

		CLegacyConfigParser expBonParser("DATA/CREXPMOD.TXT");

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
		//skeleton gets exp penalty
		objects[56].get()->addBonus(-50, BonusType::EXP_MULTIPLIER, -1);
		objects[57].get()->addBonus(-50, BonusType::EXP_MULTIPLIER, -1);
		//exp for tier >7, rank 11
		expRanks[0].push_back(147000);
		expAfterUpgrade = 75; //percent
		maxExpPerBattle[0] = maxExpPerBattle[7];
	}
}


void CCreatureHandler::loadCrExpBon(CBonusSystemNode & globalEffects)
{
	if (VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE)) 	//reading default stack experience bonuses
	{
		logGlobal->debug("\tLoading stack experience bonuses");
		auto addBonusForAllCreatures = [&](std::shared_ptr<Bonus> b) {
			auto limiter = std::make_shared<CreatureLevelLimiter>();
			b->addLimiter(limiter);
			globalEffects.addNewBonus(b);
		};
		auto addBonusForTier = [&](int tier, std::shared_ptr<Bonus> b) {
			assert(vstd::iswithin(tier, 1, 7));
			//bonuses from level 7 are given to high-level creatures too
			auto max = tier == GameConstants::CREATURES_PER_TOWN ? std::numeric_limits<int>::max() : tier + 1;
			auto limiter = std::make_shared<CreatureLevelLimiter>(tier, max);
			b->addLimiter(limiter);
			globalEffects.addNewBonus(b);
		};

		CLegacyConfigParser parser("DATA/CREXPBON.TXT");

		Bonus b; //prototype with some default properties
		b.source = BonusSource::STACK_EXPERIENCE;
		b.duration = BonusDuration::PERMANENT;
		b.valType = BonusValueType::ADDITIVE_VALUE;
		b.effectRange = BonusLimitEffect::NO_LIMIT;
		b.additionalInfo = 0;
		b.turnsRemain = 0;
		BonusList bl;

		parser.endLine();

		parser.readString(); //ignore index
		loadStackExp(b, bl, parser);
		for(const auto & b : bl)
			addBonusForAllCreatures(b); //health bonus is common for all
		parser.endLine();

		for (int i = 1; i < 7; ++i)
		{
			for (int j = 0; j < 4; ++j) //four modifiers common for tiers
			{
				parser.readString(); //ignore index
				bl.clear();
				loadStackExp(b, bl, parser);
				for(const auto & b : bl)
					addBonusForTier(i, b);
				parser.endLine();
			}
		}
		for (int j = 0; j < 4; ++j) //tier 7
		{
			parser.readString(); //ignore index
			bl.clear();
			loadStackExp(b, bl, parser);
			for(const auto & b : bl)
				addBonusForTier(7, b);
			parser.endLine();
		}
		do //parse everything that's left
		{
			auto sid = static_cast<ui32>(parser.readNumber()); //id = this particular creature ID

			b.sid = sid;
			bl.clear();
			loadStackExp(b, bl, parser);
			for(const auto & b : bl)
			{
				objects[sid]->addNewBonus(b); //add directly to CCreature Node
			}
		}
		while (parser.endLine());

	}//end of Stack Experience
}

void CCreatureHandler::loadAnimationInfo(std::vector<JsonNode> &h3Data) const
{
	CLegacyConfigParser parser("DATA/CRANIM.TXT");

	parser.endLine(); // header
	parser.endLine();

	for(int dd = 0; dd < VLC->settings()->getInteger(EGameSettings::TEXTS_CREATURE); ++dd)
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

	graphics["troopCountLocationOffset"].Float() = parser.readNumber();

	missile["attackClimaxFrame"].Float() = parser.readNumber();

	// assume that creature is not a shooter and should not have whole missile field
	if (missile["frameAngles"].Vector()[0].Float() == 0 &&
	    missile["attackClimaxFrame"].Float() == 0)
		graphics.Struct().erase("missile");
}

void CCreatureHandler::loadJsonAnimation(CCreature * cre, const JsonNode & graphics) const
{
	cre->animation.timeBetweenFidgets = graphics["timeBetweenFidgets"].Float();
	cre->animation.troopCountLocationOffset = static_cast<int>(graphics["troopCountLocationOffset"].Float());

	const JsonNode & animationTime = graphics["animationTime"];
	cre->animation.walkAnimationTime = animationTime["walk"].Float();
	cre->animation.idleAnimationTime = animationTime["idle"].Float();
	cre->animation.attackAnimationTime = animationTime["attack"].Float();

	const JsonNode & missile = graphics["missile"];
	const JsonNode & offsets = missile["offset"];
	cre->animation.upperRightMissleOffsetX = static_cast<int>(offsets["upperX"].Float());
	cre->animation.upperRightMissleOffsetY = static_cast<int>(offsets["upperY"].Float());
	cre->animation.rightMissleOffsetX =      static_cast<int>(offsets["middleX"].Float());
	cre->animation.rightMissleOffsetY =      static_cast<int>(offsets["middleY"].Float());
	cre->animation.lowerRightMissleOffsetX = static_cast<int>(offsets["lowerX"].Float());
	cre->animation.lowerRightMissleOffsetY = static_cast<int>(offsets["lowerY"].Float());

	cre->animation.attackClimaxFrame = static_cast<int>(missile["attackClimaxFrame"].Float());
	cre->animation.missleFrameAngles = missile["frameAngles"].convertTo<std::vector<double> >();

	cre->smallIconName = graphics["iconSmall"].String();
	cre->largeIconName = graphics["iconLarge"].String();
}

void CCreatureHandler::loadCreatureJson(CCreature * creature, const JsonNode & config) const
{
	creature->animDefName = config["graphics"]["animation"].String();

	//FIXME: MOD COMPATIBILITY
	if (config["abilities"].getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		for(const auto & ability : config["abilities"].Struct())
		{
			if (!ability.second.isNull())
			{
				auto b = JsonUtils::parseBonus(ability.second);
				b->source = BonusSource::CREATURE_ABILITY;
				b->sid = creature->getIndex();
				b->duration = BonusDuration::PERMANENT;
				creature->addNewBonus(b);
			}
		}
	}
	else
	{
		for(const JsonNode &ability : config["abilities"].Vector())
		{
			if(ability.getType() == JsonNode::JsonType::DATA_VECTOR)
			{
				logMod->error("Ignored outdated creature ability format in %s", creature->getJsonKey());
			}
			else
			{
				auto b = JsonUtils::parseBonus(ability);
				b->source = BonusSource::CREATURE_ABILITY;
				b->sid = creature->getIndex();
				b->duration = BonusDuration::PERMANENT;
				creature->addNewBonus(b);
			}
		}
	}

	VLC->identifiers()->requestIdentifier("faction", config["faction"], [=](si32 faction)
	{
		creature->faction = FactionID(faction);
	});

	for(const JsonNode &value : config["upgrades"].Vector())
	{
		VLC->identifiers()->requestIdentifier("creature", value, [=](si32 identifier)
		{
			creature->upgrades.insert(CreatureID(identifier));
		});
	}

	creature->animation.projectileImageName = config["graphics"]["missile"]["projectile"].String();

	for(const JsonNode & value : config["graphics"]["missile"]["ray"].Vector())
	{
		CCreature::CreatureAnimation::RayColor color;

		color.start.r = value["start"].Vector()[0].Integer();
		color.start.g = value["start"].Vector()[1].Integer();
		color.start.b = value["start"].Vector()[2].Integer();
		color.start.a = value["start"].Vector()[3].Integer();

		color.end.r = value["end"].Vector()[0].Integer();
		color.end.g = value["end"].Vector()[1].Integer();
		color.end.b = value["end"].Vector()[2].Integer();
		color.end.a = value["end"].Vector()[3].Integer();

		creature->animation.projectileRay.push_back(color);
	}

	creature->special = config["special"].Bool() || config["disabled"].Bool();

	const JsonNode & sounds = config["sound"];

#define GET_SOUND_VALUE(value_name) creature->sounds.value_name = sounds[#value_name].String()
	GET_SOUND_VALUE(attack);
	GET_SOUND_VALUE(defend);
	GET_SOUND_VALUE(killed);
	GET_SOUND_VALUE(move);
	GET_SOUND_VALUE(shoot);
	GET_SOUND_VALUE(wince);
	GET_SOUND_VALUE(startMoving);
	GET_SOUND_VALUE(endMoving);
#undef GET_SOUND_VALUE
}

void CCreatureHandler::loadStackExperience(CCreature * creature, const JsonNode & input) const
{
	for (const JsonNode &exp : input.Vector())
	{
		const JsonVector &values = exp["values"].Vector();
		int lowerLimit = 1;//, upperLimit = 255;
		if (values[0].getType() == JsonNode::JsonType::DATA_BOOL)
		{
			for (const JsonNode &val : values)
			{
				if(val.Bool())
				{
					// parse each bonus separately
					// we can not create copies since identifiers resolution does not tracks copies
					// leading to unset identifier values in copies
					auto bonus = JsonUtils::parseBonus (exp["bonus"]);
					bonus->source = BonusSource::STACK_EXPERIENCE;
					bonus->duration = BonusDuration::PERMANENT;
					bonus->limiter = std::make_shared<RankRangeLimiter>(RankRangeLimiter(lowerLimit));
					creature->addNewBonus (bonus);
					break; //TODO: allow bonuses to turn off?
				}
				++lowerLimit;
			}
		}
		else
		{
			int lastVal = 0;
			for (const JsonNode &val : values)
			{
				if (val.Float() != lastVal)
				{
					JsonNode bonusInput = exp["bonus"];
					bonusInput["val"].Float() = static_cast<int>(val.Float()) - lastVal;

					auto bonus = JsonUtils::parseBonus (bonusInput);
					bonus->source = BonusSource::STACK_EXPERIENCE;
					bonus->duration = BonusDuration::PERMANENT;
					bonus->limiter.reset (new RankRangeLimiter(lowerLimit));
					creature->addNewBonus (bonus);
				}
				lastVal = static_cast<int>(val.Float());
				++lowerLimit;
			}
		}
	}
}

void CCreatureHandler::loadStackExp(Bonus & b, BonusList & bl, CLegacyConfigParser & parser) const//help function for parsing CREXPBON.txt
{
	bool enable = false; //some bonuses are activated with values 2 or 1
	std::string buf = parser.readString();
	std::string mod = parser.readString();

	switch (buf[0])
	{
	case 'H':
		b.type = BonusType::STACK_HEALTH;
		b.valType = BonusValueType::PERCENT_TO_BASE;
		break;
	case 'A':
		b.type = BonusType::PRIMARY_SKILL;
		b.subtype = PrimarySkill::ATTACK;
		break;
	case 'D':
		b.type = BonusType::PRIMARY_SKILL;
		b.subtype = PrimarySkill::DEFENSE;
		break;
	case 'M': //Max damage
		b.type = BonusType::CREATURE_DAMAGE;
		b.subtype = 2;
		break;
	case 'm': //Min damage
		b.type = BonusType::CREATURE_DAMAGE;
		b.subtype = 1;
		break;
	case 'S':
		b.type = BonusType::STACKS_SPEED; break;
	case 'O':
		b.type = BonusType::SHOTS; break;
	case 'b':
		b.type = BonusType::ENEMY_DEFENCE_REDUCTION; break;
	case 'C':
		b.type = BonusType::CHANGES_SPELL_COST_FOR_ALLY; break;
	case 'd':
		b.type = BonusType::DEFENSIVE_STANCE; break;
	case 'e':
		b.type = BonusType::DOUBLE_DAMAGE_CHANCE;
		b.subtype = 0;
		break;
	case 'E':
		b.type = BonusType::DEATH_STARE;
		b.subtype = 0; //Gorgon
		break;
	case 'F':
		b.type = BonusType::FEAR; break;
	case 'g':
		b.type = BonusType::SPELL_DAMAGE_REDUCTION;
		b.subtype = SpellSchool(ESpellSchool::ANY);
		break;
	case 'P':
		b.type = BonusType::CASTS; break;
	case 'R':
		b.type = BonusType::ADDITIONAL_RETALIATION; break;
	case 'W':
		b.type = BonusType::MAGIC_RESISTANCE;
		b.subtype = 0; //otherwise creature window goes crazy
		break;
	case 'f': //on-off skill
		enable = true; //sometimes format is: 2 -> 0, 1 -> 1
		switch (mod[0])
		{
			case 'A':
				b.type = BonusType::ATTACKS_ALL_ADJACENT; break;
			case 'b':
				b.type = BonusType::RETURN_AFTER_STRIKE; break;
			case 'B':
				b.type = BonusType::TWO_HEX_ATTACK_BREATH; break;
			case 'c':
				b.type = BonusType::JOUSTING; 
				b.val = 5;
				break;
			case 'D':
				b.type = BonusType::ADDITIONAL_ATTACK; break;
			case 'f':
				b.type = BonusType::FEARLESS; break;
			case 'F':
				b.type = BonusType::FLYING; break;
			case 'm':
				b.type = BonusType::MORALE;
				b.val = 1;
				b.valType = BonusValueType::INDEPENDENT_MAX;
				break;
			case 'M':
				b.type = BonusType::NO_MORALE; break;
			case 'p': //Mind spells
			case 'P':
				b.type = BonusType::MIND_IMMUNITY; break;
			case 'r':
				b.type = BonusType::REBIRTH; //on/off? makes sense?
				b.subtype = 0;
				b.val = 20; //arbitrary value
				break;
			case 'R':
				b.type = BonusType::BLOCKS_RETALIATION; break;
			case 's':
				b.type = BonusType::FREE_SHOOTING; break;
			case 'u':
				b.type = BonusType::SPELL_RESISTANCE_AURA; break;
			case 'U':
				b.type = BonusType::UNDEAD; break;
			default:
				logGlobal->trace("Not parsed bonus %s %s", buf, mod);
				return;
				break;
		}
		break;
	case 'w': //specific spell immunities, enabled/disabled
		enable = true;
		switch (mod[0])
		{
			case 'B': //Blind
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::BLIND;
				b.additionalInfo = 0;//normal immunity
				break;
			case 'H': //Hypnotize
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::HYPNOTIZE;
				b.additionalInfo = 0;//normal immunity
				break;
			case 'I': //Implosion
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::IMPLOSION;
				b.additionalInfo = 0;//normal immunity
				break;
			case 'K': //Berserk
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::BERSERK;
				b.additionalInfo = 0;//normal immunity
				break;
			case 'M': //Meteor Shower
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::METEOR_SHOWER;
				b.additionalInfo = 0;//normal immunity
				break;
			case 'N': //dispell beneficial spells
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::DISPEL_HELPFUL_SPELLS;
				b.additionalInfo = 0;//normal immunity
				break;
			case 'R': //Armageddon
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::ARMAGEDDON;
				b.additionalInfo = 0;//normal immunity
				break;
			case 'S': //Slow
				b.type = BonusType::SPELL_IMMUNITY;
				b.subtype = SpellID::SLOW;
				b.additionalInfo = 0;//normal immunity
				break;
			case '6':
			case '7':
			case '8':
			case '9':
				b.type = BonusType::LEVEL_SPELL_IMMUNITY;
				b.val = std::atoi(mod.c_str()) - 5;
				break;
			case ':':
				b.type = BonusType::LEVEL_SPELL_IMMUNITY;
				b.val = GameConstants::SPELL_LEVELS; //in case someone adds higher level spells?
				break;
			case 'F':
				b.type = BonusType::NEGATIVE_EFFECTS_IMMUNITY;
				b.subtype = SpellSchool(ESpellSchool::FIRE); 
				break;
			case 'O':
				b.type = BonusType::SPELL_DAMAGE_REDUCTION;
				b.subtype = SpellSchool(ESpellSchool::FIRE);
				b.val = 100; //Full damage immunity
				break;
			case 'f':
				b.type = BonusType::SPELL_SCHOOL_IMMUNITY;
				b.subtype = SpellSchool(ESpellSchool::FIRE); 
				break;
			case 'C':
				b.type = BonusType::NEGATIVE_EFFECTS_IMMUNITY;
				b.subtype = SpellSchool(ESpellSchool::WATER);
				break;
			case 'W':
				b.type = BonusType::SPELL_DAMAGE_REDUCTION;
				b.subtype = SpellSchool(ESpellSchool::WATER);
				b.val = 100; //Full damage immunity
				break;
			case 'w':
				b.type = BonusType::SPELL_SCHOOL_IMMUNITY;
				b.subtype = SpellSchool(ESpellSchool::WATER);
				break;
			case 'E':
				b.type = BonusType::SPELL_DAMAGE_REDUCTION;
				b.subtype = SpellSchool(ESpellSchool::EARTH);
				b.val = 100; //Full damage immunity
				break;
			case 'e':
				b.type = BonusType::SPELL_SCHOOL_IMMUNITY;
				b.subtype = SpellSchool(ESpellSchool::EARTH);
				break;
			case 'A':
				b.type = BonusType::SPELL_DAMAGE_REDUCTION;
				b.subtype = SpellSchool(ESpellSchool::AIR);
				b.val = 100; //Full damage immunity
				break;
			case 'a':
				b.type = BonusType::SPELL_SCHOOL_IMMUNITY;
				b.subtype = SpellSchool(ESpellSchool::AIR);
				break;
			case 'D':
				b.type = BonusType::SPELL_DAMAGE_REDUCTION;
				b.subtype = SpellSchool(ESpellSchool::ANY);
				b.val = 100; //Full damage immunity
				break;
			case '0':
				b.type = BonusType::RECEPTIVE;
				break;
			case 'm':
				b.type = BonusType::MIND_IMMUNITY;
				break;
			default:
				logGlobal->trace("Not parsed bonus %s %s", buf, mod);
				return;
		}
		break;

	case 'i':
		enable = true;
		b.type = BonusType::NO_DISTANCE_PENALTY;
		break;
	case 'o':
		enable = true;
		b.type = BonusType::NO_WALL_PENALTY;
		break;

	case 'a':
	case 'c':
	case 'K':
	case 'k':
		b.type = BonusType::SPELL_AFTER_ATTACK;
		b.subtype = stringToNumber(mod);
		break;
	case 'h':
		b.type = BonusType::HATE;
		b.subtype = stringToNumber(mod);
		break;
	case 'p':
	case 'J':
		b.type = BonusType::SPELL_BEFORE_ATTACK;
		b.subtype = stringToNumber(mod);
		b.additionalInfo = 3; //always expert?
		break;
	case 'r':
		b.type = BonusType::HP_REGENERATION;
		b.val = stringToNumber(mod);
		break;
	case 's':
		b.type = BonusType::ENCHANTED;
		b.subtype = stringToNumber(mod);
		b.valType = BonusValueType::INDEPENDENT_MAX;
		break;
	default:
		logGlobal->trace("Not parsed bonus %s %s", buf, mod);
		return;
		break;
	}
	switch (mod[0])
	{
		case '+':
		case '=': //should we allow percent values to stack or pick highest?
			b.valType = BonusValueType::ADDITIVE_VALUE;
			break;
	}

	//limiters, range
	si32 lastVal;
	si32 curVal;
	si32 lastLev = 0;

	if (enable) //0 and 2 means non-active, 1 - active
	{
		if (b.type != BonusType::REBIRTH)
			b.val = 0; //on-off ability, no value specified
		parser.readNumber(); // 0 level is never active
		for (int i = 1; i < 11; ++i)
		{
			curVal = static_cast<si32>(parser.readNumber());
			if (curVal == 1)
			{
				b.limiter.reset (new RankRangeLimiter(i));
				bl.push_back(std::make_shared<Bonus>(b));
				break; //never turned off it seems
			}
		}
	}
	else
	{
		lastVal = static_cast<si32>(parser.readNumber());
		if (b.type == BonusType::HATE)
			lastVal *= 10; //odd fix
		//FIXME: value for zero level should be stored in our config files (independent of stack exp)
		for (int i = 1; i < 11; ++i)
		{
			curVal = static_cast<si32>(parser.readNumber());
			if (b.type == BonusType::HATE)
				curVal *= 10; //odd fix
			if (curVal > lastVal) //threshold, add new bonus
			{
				b.val = curVal - lastVal;
				lastVal = curVal;
				b.limiter.reset (new RankRangeLimiter(i));
				bl.push_back(std::make_shared<Bonus>(b));
				lastLev = i; //start new range from here, i = previous rank
			}
			else if (curVal < lastVal)
			{
				b.val = lastVal;
				b.limiter.reset (new RankRangeLimiter(lastLev, i));
			}
		}
	}
}

int CCreatureHandler::stringToNumber(std::string & s) const
{
	boost::algorithm::replace_first(s,"#",""); //drop hash character
	return std::atoi(s.c_str());
}

CCreatureHandler::~CCreatureHandler()
{
	for(auto & p : skillRequirements)
		p.first = nullptr;
}

CreatureID CCreatureHandler::pickRandomMonster(CRandomGenerator & rand, int tier) const
{
	int r = 0;
	if(tier == -1) //pick any allowed creature
	{
		do
		{
			r = (*RandomGeneratorUtil::nextItem(objects, rand))->getId();
		} while (objects[r] && objects[r]->special); // find first "not special" creature
	}
	else
	{
		assert(vstd::iswithin(tier, 1, 7));
		std::vector<CreatureID> allowed;
		for(const auto & creature : objects)
		{
			if(!creature->special && creature->level == tier)
				allowed.push_back(creature->getId());
		}

		if(allowed.empty())
		{
			logGlobal->warn("Cannot pick a random creature of tier %d!", tier);
			return CreatureID::NONE;
		}

		return *RandomGeneratorUtil::nextItem(allowed, rand);
	}
	assert (r >= 0); //should always be, but it crashed
	return CreatureID(r);
}


void CCreatureHandler::afterLoadFinalization()
{

}

VCMI_LIB_NAMESPACE_END
