/*
 * CHeroHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroHandler.h"

#include "CGeneralTextHandler.h"
#include "filesystem/Filesystem.h"
#include "VCMI_Lib.h"
#include "JsonNode.h"
#include "StringConstants.h"
#include "battle/BattleHex.h"
#include "CCreatureHandler.h"
#include "CModHandler.h"
#include "CTownHandler.h"
#include "mapObjects/CObjectHandler.h" //for hero specialty
#include "CSkillHandler.h"
#include <math.h>

#include "mapObjects/CObjectClassesHandler.h"

CHero::CHero() = default;
CHero::~CHero() = default;

int32_t CHero::getIndex() const
{
	return ID.getNum();
}

const std::string & CHero::getName() const
{
	return name;
}

const std::string & CHero::getJsonKey() const
{
	return identifier;
}

HeroTypeID CHero::getId() const
{
	return ID;
}

void CHero::registerIcons(const IconRegistar & cb) const
{
	cb(imageIndex, "UN32", iconSpecSmall);
	cb(imageIndex, "UN44", iconSpecLarge);
	cb(imageIndex, "PORTRAITSLARGE", portraitLarge);
	cb(imageIndex, "PORTRAITSSMALL", portraitSmall);
}

SecondarySkill CHeroClass::chooseSecSkill(const std::set<SecondarySkill> & possibles, CRandomGenerator & rand) const //picks secondary skill out from given possibilities
{
	int totalProb = 0;
	for(auto & possible : possibles)
	{
		totalProb += secSkillProbability[possible];
	}
	if (totalProb != 0) // may trigger if set contains only banned skills (0 probability)
	{
		auto ran = rand.nextInt(totalProb - 1);
		for(auto & possible : possibles)
		{
			ran -= secSkillProbability[possible];
			if(ran < 0)
			{
				return possible;
			}
		}
	}
	// FIXME: select randomly? How H3 handles such rare situation?
	return *possibles.begin();
}

bool CHeroClass::isMagicHero() const
{
	return affinity == MAGIC;
}

EAlignment::EAlignment CHeroClass::getAlignment() const
{
	return EAlignment::EAlignment(VLC->townh->factions[faction]->alignment);
}

int32_t CHeroClass::getIndex() const
{
	return id.getNum();
}

const std::string & CHeroClass::getName() const
{
	return name;
}

const std::string & CHeroClass::getJsonKey() const
{
	return identifier;
}

HeroClassID CHeroClass::getId() const
{
	return id;
}

void CHeroClass::registerIcons(const IconRegistar & cb) const
{

}

CHeroClass::CHeroClass()
 : faction(0), id(), affinity(0), defaultTavernChance(0), commander(nullptr)
{
}

std::vector<BattleHex> CObstacleInfo::getBlocked(BattleHex hex) const
{
	std::vector<BattleHex> ret;
	if(isAbsoluteObstacle)
	{
		assert(!hex.isValid());
		range::copy(blockedTiles, std::back_inserter(ret));
		return ret;
	}

	for(int offset : blockedTiles)
	{
		BattleHex toBlock = hex + offset;
		if((hex.getY() & 1) && !(toBlock.getY() & 1))
			toBlock += BattleHex::LEFT;

		if(!toBlock.isValid())
			logGlobal->error("Misplaced obstacle!");
		else
			ret.push_back(toBlock);
	}

	return ret;
}

bool CObstacleInfo::isAppropriate(ETerrainType terrainType, int specialBattlefield) const
{
	if(specialBattlefield != -1)
		return vstd::contains(allowedSpecialBfields, specialBattlefield);

	return vstd::contains(allowedTerrains, terrainType);
}

CHeroClass * CHeroClassHandler::loadFromJson(const JsonNode & node, const std::string & identifier)
{
	std::string affinityStr[2] = { "might", "magic" };

	auto  heroClass = new CHeroClass();
	heroClass->identifier = identifier;
	heroClass->imageBattleFemale = node["animation"]["battle"]["female"].String();
	heroClass->imageBattleMale   = node["animation"]["battle"]["male"].String();
	//MODS COMPATIBILITY FOR 0.96
	heroClass->imageMapFemale    = node["animation"]["map"]["female"].String();
	heroClass->imageMapMale      = node["animation"]["map"]["male"].String();

	heroClass->name = node["name"].String();
	heroClass->affinity = vstd::find_pos(affinityStr, node["affinity"].String());

	for(const std::string & pSkill : PrimarySkill::names)
	{
		heroClass->primarySkillInitial.push_back(node["primarySkills"][pSkill].Float());
		heroClass->primarySkillLowLevel.push_back(node["lowLevelChance"][pSkill].Float());
		heroClass->primarySkillHighLevel.push_back(node["highLevelChance"][pSkill].Float());
	}

	for(auto skillPair : node["secondarySkills"].Struct())
	{
		int probability = skillPair.second.Integer();
		VLC->modh->identifiers.requestIdentifier(skillPair.second.meta, "skill", skillPair.first, [heroClass, probability](si32 skillID)
		{
			if(heroClass->secSkillProbability.size() <= skillID)
				heroClass->secSkillProbability.resize(skillID + 1, -1); // -1 = override with default later
			heroClass->secSkillProbability[skillID] = probability;
		});
	}

	VLC->modh->identifiers.requestIdentifier ("creature", node["commander"],
	[=](si32 commanderID)
	{
		heroClass->commander = VLC->creh->creatures[commanderID];
	});

	heroClass->defaultTavernChance = node["defaultTavern"].Float();
	for(auto & tavern : node["tavern"].Struct())
	{
		int value = tavern.second.Float();

		VLC->modh->identifiers.requestIdentifier(tavern.second.meta, "faction", tavern.first,
		[=](si32 factionID)
		{
			heroClass->selectionProbability[factionID] = value;
		});
	}

	VLC->modh->identifiers.requestIdentifier("faction", node["faction"],
	[=](si32 factionID)
	{
		heroClass->faction = factionID;
	});

	return heroClass;
}

std::vector<JsonNode> CHeroClassHandler::loadLegacyData(size_t dataSize)
{
	heroClasses.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser parser("DATA/HCTRAITS.TXT");

	parser.endLine(); // header
	parser.endLine();

	for (size_t i=0; i<dataSize; i++)
	{
		JsonNode entry;

		entry["name"].String() = parser.readString();

		parser.readNumber(); // unused aggression

		for (auto & name : PrimarySkill::names)
			entry["primarySkills"][name].Float() = parser.readNumber();

		for (auto & name : PrimarySkill::names)
			entry["lowLevelChance"][name].Float() = parser.readNumber();

		for (auto & name : PrimarySkill::names)
			entry["highLevelChance"][name].Float() = parser.readNumber();

		for (auto & name : NSecondarySkill::names)
			entry["secondarySkills"][name].Float() = parser.readNumber();

		for(auto & name : ETownType::names)
			entry["tavern"][name].Float() = parser.readNumber();

		parser.endLine();
		h3Data.push_back(entry);
	}
	return h3Data;
}

void CHeroClassHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));
	object->id = HeroClassID(heroClasses.size());

	heroClasses.push_back(object);

	VLC->modh->identifiers.requestIdentifier(scope, "object", "hero", [=](si32 index)
	{
		JsonNode classConf = data["mapObject"];
		classConf["heroClass"].String() = name;
		classConf.setMeta(scope);
		VLC->objtypeh->loadSubObject(name, classConf, index, object->getIndex());
	});

	VLC->modh->identifiers.registerObject(scope, "heroClass", name, object->getIndex());
}

void CHeroClassHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));
	object->id = HeroClassID(index);

	assert(heroClasses[index] == nullptr); // ensure that this id was not loaded before
	heroClasses[index] = object;

	VLC->modh->identifiers.requestIdentifier(scope, "object", "hero", [=](si32 index)
	{
		JsonNode classConf = data["mapObject"];
		classConf["heroClass"].String() = name;
		classConf.setMeta(scope);
		VLC->objtypeh->loadSubObject(name, classConf, index, object->getIndex());
	});

	VLC->modh->identifiers.registerObject(scope, "heroClass", name, object->getIndex());
}

void CHeroClassHandler::afterLoadFinalization()
{
	// for each pair <class, town> set selection probability if it was not set before in tavern entries
	for (CHeroClass * heroClass : heroClasses)
	{
		for (CFaction * faction : VLC->townh->factions)
		{
			if (!faction->town)
				continue;
			if (heroClass->selectionProbability.count(faction->index))
				continue;

			float chance = heroClass->defaultTavernChance * faction->town->defaultTavernChance;
			heroClass->selectionProbability[faction->index] = static_cast<int>(sqrt(chance) + 0.5); //FIXME: replace with std::round once MVS supports it
		}
		// set default probabilities for gaining secondary skills where not loaded previously
		heroClass->secSkillProbability.resize(VLC->skillh->size(), -1);
		for(int skillID = 0; skillID < VLC->skillh->size(); skillID++)
		{
			if(heroClass->secSkillProbability[skillID] < 0)
			{
				const CSkill * skill = (*VLC->skillh)[SecondarySkill(skillID)];
				logMod->trace("%s: no probability for %s, using default", heroClass->identifier, skill->identifier);
				heroClass->secSkillProbability[skillID] = skill->gainChance[heroClass->affinity];
			}
		}
	}

	for (CHeroClass * hc : heroClasses)
	{
		if (!hc->imageMapMale.empty())
		{
			JsonNode templ;
			templ["animation"].String() = hc->imageMapMale;
			VLC->objtypeh->getHandlerFor(Obj::HERO, hc->getIndex())->addTemplate(templ);
		}
	}
}

const Entity * CHeroClassHandler::getBaseByIndex(const int32_t index) const
{
	return getByIndex(index);
}

const HeroClass * CHeroClassHandler::getById(const HeroClassID & id) const
{
	return getByIndex(id.getNum());
}

const HeroClass * CHeroClassHandler::getByIndex(const int32_t index) const
{
	if(index < 0 || index >= heroClasses.size())
	{
		logGlobal->error("Unable to get hero with ID %d", int32_t(index));
		return nullptr;
	}
	else
	{
		return heroClasses.at(index).get();
	}
}

void CHeroClassHandler::forEachBase(const std::function<void(const Entity *, bool &)>& cb) const
{
	bool stop = false;

	for(auto & object : heroClasses)
	{
		cb(object.get(), stop);
		if(stop)
			break;
	}
}

void CHeroClassHandler::forEach(const std::function<void(const HeroClass *, bool &)>& cb) const
{
	bool stop = false;

	for(auto & object : heroClasses)
	{
		cb(object.get(), stop);
		if(stop)
			break;
	}
}

std::vector<bool> CHeroClassHandler::getDefaultAllowed() const
{
	return std::vector<bool>(heroClasses.size(), true);
}

CHeroClassHandler::~CHeroClassHandler()
{
	for(auto heroClass : heroClasses)
	{
		delete heroClass.get();
	}
}

CHeroHandler::~CHeroHandler()
{
	for(auto hero : heroes)
		delete hero.get();
}

CHeroHandler::CHeroHandler()
{
	VLC->heroh = this;

	loadObstacles();
	loadTerrains();
	for (int i = 0; i < GameConstants::TERRAIN_TYPES; ++i)
	{
		VLC->modh->identifiers.registerObject("core", "terrain", GameConstants::TERRAIN_NAMES[i], i);
	}
	loadBallistics();
	loadExperience();
}

CHero * CHeroHandler::loadFromJson(const JsonNode & node, const std::string & identifier)
{
	auto hero = new CHero();
	hero->identifier = identifier;
	hero->sex = node["female"].Bool();
	hero->special = node["special"].Bool();

	hero->name        = node["texts"]["name"].String();
	hero->biography   = node["texts"]["biography"].String();
	hero->specName    = node["texts"]["specialty"]["name"].String();
	hero->specTooltip = node["texts"]["specialty"]["tooltip"].String();
	hero->specDescr   = node["texts"]["specialty"]["description"].String();

	hero->iconSpecSmall = node["images"]["specialtySmall"].String();
	hero->iconSpecLarge = node["images"]["specialtyLarge"].String();
	hero->portraitSmall = node["images"]["small"].String();
	hero->portraitLarge = node["images"]["large"].String();
	hero->battleImage = node["battleImage"].String();

	loadHeroArmy(hero, node);
	loadHeroSkills(hero, node);
	loadHeroSpecialty(hero, node);

	VLC->modh->identifiers.requestIdentifier("heroClass", node["class"],
	[=](si32 classID)
	{
		hero->heroClass = classes.heroClasses[classID];
	});

	return hero;
}

void CHeroHandler::loadHeroArmy(CHero * hero, const JsonNode & node)
{
	assert(node["army"].Vector().size() <= 3); // anything bigger is useless - army initialization uses up to 3 slots

	hero->initialArmy.resize(node["army"].Vector().size());

	for (size_t i=0; i< hero->initialArmy.size(); i++)
	{
		const JsonNode & source = node["army"].Vector()[i];

		hero->initialArmy[i].minAmount = source["min"].Float();
		hero->initialArmy[i].maxAmount = source["max"].Float();

		assert(hero->initialArmy[i].minAmount <= hero->initialArmy[i].maxAmount);

		VLC->modh->identifiers.requestIdentifier("creature", source["creature"], [=](si32 creature)
		{
			hero->initialArmy[i].creature = CreatureID(creature);
		});
	}
}

void CHeroHandler::loadHeroSkills(CHero * hero, const JsonNode & node)
{
	for(const JsonNode &set : node["skills"].Vector())
	{
		int skillLevel = boost::range::find(NSecondarySkill::levels, set["level"].String()) - std::begin(NSecondarySkill::levels);
		if (skillLevel < SecSkillLevel::LEVELS_SIZE)
		{
			size_t currentIndex = hero->secSkillsInit.size();
			hero->secSkillsInit.push_back(std::make_pair(SecondarySkill(-1), skillLevel));

			VLC->modh->identifiers.requestIdentifier("skill", set["skill"], [=](si32 id)
			{
				hero->secSkillsInit[currentIndex].first = SecondarySkill(id);
			});
		}
		else
		{
			logMod->error("Unknown skill level: %s", set["level"].String());
		}
	}

	// spellbook is considered present if hero have "spellbook" entry even when this is an empty set (0 spells)
	hero->haveSpellBook = !node["spellbook"].isNull();

	for(const JsonNode & spell : node["spellbook"].Vector())
	{
		VLC->modh->identifiers.requestIdentifier("spell", spell,
		[=](si32 spellID)
		{
			hero->spells.insert(SpellID(spellID));
		});
	}
}

// add standard creature specialty to result
void AddSpecialtyForCreature(int creatureID, std::shared_ptr<Bonus> bonus, std::vector<std::shared_ptr<Bonus>> &result)
{
	const CCreature &specBaseCreature = *VLC->creh->creatures[creatureID]; //base creature in which we have specialty

	bonus->limiter.reset(new CCreatureTypeLimiter(specBaseCreature, true));
	bonus->type = Bonus::STACKS_SPEED;
	bonus->valType = Bonus::ADDITIVE_VALUE;
	bonus->val = 1;
	result.push_back(bonus);

	// attack and defense may differ for upgraded creatures => separate bonuses
	std::vector<int> specTargets;
	specTargets.push_back(creatureID);
	specTargets.insert(specTargets.end(), specBaseCreature.upgrades.begin(), specBaseCreature.upgrades.end());

	for(int cid : specTargets)
	{
		const CCreature &specCreature = *VLC->creh->creatures[cid];
		bonus = std::make_shared<Bonus>(*bonus);
		bonus->limiter.reset(new CCreatureTypeLimiter(specCreature, false));
		bonus->type = Bonus::PRIMARY_SKILL;
		bonus->val = 0;

		int stepSize = specCreature.level ? specCreature.level : 5;

		bonus->subtype = PrimarySkill::ATTACK;
		bonus->updater.reset(new GrowsWithLevelUpdater(specCreature.getAttack(false), stepSize));
		result.push_back(bonus);

		bonus = std::make_shared<Bonus>(*bonus);
		bonus->subtype = PrimarySkill::DEFENSE;
		bonus->updater.reset(new GrowsWithLevelUpdater(specCreature.getDefence(false), stepSize));
		result.push_back(bonus);
	}
}

// convert deprecated format
std::vector<std::shared_ptr<Bonus>> SpecialtyInfoToBonuses(const SSpecialtyInfo & spec, int sid)
{
	std::vector<std::shared_ptr<Bonus>> result;

	std::shared_ptr<Bonus> bonus = std::make_shared<Bonus>();
	bonus->duration = Bonus::PERMANENT;
	bonus->source = Bonus::HERO_SPECIAL;
	bonus->sid = sid;
	bonus->val = spec.val;

	switch (spec.type)
	{
	case 1: //creature specialty
		AddSpecialtyForCreature(spec.additionalinfo, bonus, result);
		break;
	case 2: //secondary skill
		bonus->type = Bonus::SECONDARY_SKILL_PREMY;
		bonus->valType = Bonus::PERCENT_TO_BASE;
		bonus->subtype = spec.subtype;
		bonus->updater.reset(new TimesHeroLevelUpdater());
		result.push_back(bonus);
		break;
	case 3: //spell damage bonus, level dependent but calculated elsewhere
		bonus->type = Bonus::SPECIAL_SPELL_LEV;
		bonus->subtype = spec.subtype;
		bonus->updater.reset(new TimesHeroLevelUpdater());
		result.push_back(bonus);
		break;
	case 4: //creature stat boost
		switch (spec.subtype)
		{
		case 1:
			bonus->type = Bonus::PRIMARY_SKILL;
			bonus->subtype = PrimarySkill::ATTACK;
			break;
		case 2:
			bonus->type = Bonus::PRIMARY_SKILL;
			bonus->subtype = PrimarySkill::DEFENSE;
			break;
		case 3:
			bonus->type = Bonus::CREATURE_DAMAGE;
			bonus->subtype = 0; //both min and max
			break;
		case 4:
			bonus->type = Bonus::STACK_HEALTH;
			break;
		case 5:
			bonus->type = Bonus::STACKS_SPEED;
			break;
		default:
			logMod->warn("Unknown subtype for specialty 4");
			return result;
		}
		bonus->valType = Bonus::ADDITIVE_VALUE;
		bonus->limiter.reset(new CCreatureTypeLimiter(*VLC->creh->creatures[spec.additionalinfo], true));
		result.push_back(bonus);
		break;
	case 5: //spell damage bonus in percent
		bonus->type = Bonus::SPECIFIC_SPELL_DAMAGE;
		bonus->valType = Bonus::BASE_NUMBER; //current spell system is screwed
		bonus->subtype = spec.subtype; //spell id
		result.push_back(bonus);
		break;
	case 6: //damage bonus for bless (Adela)
		bonus->type = Bonus::SPECIAL_BLESS_DAMAGE;
		bonus->subtype = spec.subtype; //spell id if you ever wanted to use it otherwise
		bonus->additionalInfo = spec.additionalinfo; //damage factor
		bonus->updater.reset(new TimesHeroLevelUpdater());
		result.push_back(bonus);
		break;
	case 7: //maxed mastery for spell
		bonus->type = Bonus::MAXED_SPELL;
		bonus->subtype = spec.subtype; //spell id
		result.push_back(bonus);
		break;
	case 8: //peculiar spells - enchantments
		bonus->type = Bonus::SPECIAL_PECULIAR_ENCHANT;
		bonus->subtype = spec.subtype; //spell id
		bonus->additionalInfo = spec.additionalinfo; //0, 1 for Coronius
		result.push_back(bonus);
		break;
	case 9: //upgrade creatures
		{
			const auto &creatures = VLC->creh->creatures;
			bonus->type = Bonus::SPECIAL_UPGRADE;
			bonus->subtype = spec.subtype; //base id
			bonus->additionalInfo = spec.additionalinfo; //target id
			result.push_back(bonus);
			//propagate for regular upgrades of base creature
			for(auto cre_id : creatures[spec.subtype]->upgrades)
			{
				std::shared_ptr<Bonus> upgradeUpgradedVersion = std::make_shared<Bonus>(*bonus);
				upgradeUpgradedVersion->subtype = cre_id;
				result.push_back(upgradeUpgradedVersion);
			}
		}
		break;
	case 10: //resource generation
		bonus->type = Bonus::GENERATE_RESOURCE;
		bonus->subtype = spec.subtype;
		result.push_back(bonus);
		break;
	case 11: //starting skill with mastery (Adrienne)
		logMod->warn("Secondary skill mastery is no longer supported as specialty.");
		break;
	case 12: //army speed
		bonus->type = Bonus::STACKS_SPEED;
		result.push_back(bonus);
		break;
	case 13: //Dragon bonuses (Mutare)
		bonus->type = Bonus::PRIMARY_SKILL;
		bonus->valType = Bonus::ADDITIVE_VALUE;
		switch(spec.subtype)
		{
		case 1:
			bonus->subtype = PrimarySkill::ATTACK;
			break;
		case 2:
			bonus->subtype = PrimarySkill::DEFENSE;
			break;
		}
		bonus->limiter.reset(new HasAnotherBonusLimiter(Bonus::DRAGON_NATURE));
		result.push_back(bonus);
		break;
	default:
		logMod->warn("Unknown hero specialty %d", spec.type);
		break;
	}

	return result;
}

// convert deprecated format
std::vector<std::shared_ptr<Bonus>> SpecialtyBonusToBonuses(const SSpecialtyBonus & spec, int sid)
{
	std::vector<std::shared_ptr<Bonus>> result;
	for(std::shared_ptr<Bonus> oldBonus : spec.bonuses)
	{
		oldBonus->sid = sid;
		if(oldBonus->type == Bonus::SPECIAL_SPELL_LEV || oldBonus->type == Bonus::SPECIAL_BLESS_DAMAGE)
		{
			// these bonuses used to auto-scale with hero level
			std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*oldBonus);
			newBonus->updater = std::make_shared<TimesHeroLevelUpdater>();
			result.push_back(newBonus);
		}
		else if(spec.growsWithLevel)
		{
			std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*oldBonus);
			switch(newBonus->type)
			{
			case Bonus::SECONDARY_SKILL_PREMY:
				break; // ignore - used to be overwritten based on SPECIAL_SECONDARY_SKILL
			case Bonus::SPECIAL_SECONDARY_SKILL:
				newBonus->type = Bonus::SECONDARY_SKILL_PREMY;
				newBonus->updater = std::make_shared<TimesHeroLevelUpdater>();
				result.push_back(newBonus);
				break;
			case Bonus::PRIMARY_SKILL:
				if((newBonus->subtype == PrimarySkill::ATTACK || newBonus->subtype == PrimarySkill::DEFENSE) && newBonus->limiter)
				{
					const std::shared_ptr<CCreatureTypeLimiter> creatureLimiter = std::dynamic_pointer_cast<CCreatureTypeLimiter>(newBonus->limiter);
					if(creatureLimiter)
					{
						const CCreature * cre = creatureLimiter->creature;
						int creStat = newBonus->subtype == PrimarySkill::ATTACK ? cre->getAttack(false) : cre->getDefence(false);
						int creLevel = cre->level ? cre->level : 5;
						newBonus->updater = std::make_shared<GrowsWithLevelUpdater>(creStat, creLevel);
					}
					result.push_back(newBonus);
				}
				break;
			default:
				result.push_back(newBonus);
			}
		}
		else
		{
			result.push_back(oldBonus);
		}
	}
	return result;
}

void CHeroHandler::beforeValidate(JsonNode & object)
{
	//handle "base" specialty info
	JsonNode & specialtyNode = object["specialty"];
	if(specialtyNode.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		const JsonNode & base = specialtyNode["base"];
		if(!base.isNull())
		{
			if(specialtyNode["bonuses"].isNull())
			{
				logMod->warn("specialty has base without bonuses");
			}
			else
			{
				JsonMap & bonuses = specialtyNode["bonuses"].Struct();
				for(std::pair<std::string, JsonNode> keyValue : bonuses)
					JsonUtils::inherit(bonuses[keyValue.first], base);
			}
		}
	}
}

void CHeroHandler::loadHeroSpecialty(CHero * hero, const JsonNode & node)
{
	int sid = hero->ID.getNum();
	auto prepSpec = [=](std::shared_ptr<Bonus> bonus)
	{
		bonus->duration = Bonus::PERMANENT;
		bonus->source = Bonus::HERO_SPECIAL;
		bonus->sid = sid;
		return bonus;
	};

	//deprecated, used only for original specialties
	const JsonNode & specialtiesNode = node["specialties"];
	if (!specialtiesNode.isNull())
	{
		logMod->warn("Hero %s has deprecated specialties format.", hero->identifier);
		for(const JsonNode &specialty : specialtiesNode.Vector())
		{
			SSpecialtyInfo spec;
			spec.type = specialty["type"].Integer();
			spec.val = specialty["val"].Integer();
			spec.subtype = specialty["subtype"].Integer();
			spec.additionalinfo = specialty["info"].Integer();
			//we convert after loading completes, to have all identifiers for json logging
			hero->specDeprecated.push_back(spec);
		}
	}
	//new(er) format, using bonus system
	const JsonNode & specialtyNode = node["specialty"];
	if(specialtyNode.getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		//deprecated middle-aged format
		for(const JsonNode & specialty : node["specialty"].Vector())
		{
			SSpecialtyBonus hs;
			hs.growsWithLevel = specialty["growsWithLevel"].Bool();
			for (const JsonNode & bonus : specialty["bonuses"].Vector())
				hs.bonuses.push_back(prepSpec(JsonUtils::parseBonus(bonus)));
			hero->specialtyDeprecated.push_back(hs);
		}
	}
	else if(specialtyNode.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		//creature specialty - alias for simplicity
		if(!specialtyNode["creature"].isNull())
		{
			VLC->modh->identifiers.requestIdentifier("creature", specialtyNode["creature"], [hero](si32 creature) {
				// use legacy format for delayed conversion (must have all creature data loaded, also for upgrades)
				SSpecialtyInfo spec;
				spec.type = 1;
				spec.additionalinfo = creature;
				hero->specDeprecated.push_back(spec);
			});
		}
		if(!specialtyNode["bonuses"].isNull())
		{
			//proper new format
			for(auto keyValue : specialtyNode["bonuses"].Struct())
				hero->specialty.push_back(prepSpec(JsonUtils::parseBonus(keyValue.second)));
		}
	}
}

void CHeroHandler::loadExperience()
{
	expPerLevel.push_back(0);
	expPerLevel.push_back(1000);
	expPerLevel.push_back(2000);
	expPerLevel.push_back(3200);
	expPerLevel.push_back(4600);
	expPerLevel.push_back(6200);
	expPerLevel.push_back(8000);
	expPerLevel.push_back(10000);
	expPerLevel.push_back(12200);
	expPerLevel.push_back(14700);
	expPerLevel.push_back(17500);
	expPerLevel.push_back(20600);
	expPerLevel.push_back(24320);
	expPerLevel.push_back(28784);
	expPerLevel.push_back(34140);
	while (expPerLevel[expPerLevel.size() - 1] > expPerLevel[expPerLevel.size() - 2])
	{
		auto i = expPerLevel.size() - 1;
		auto diff = expPerLevel[i] - expPerLevel[i-1];
		diff += diff / 5;
		expPerLevel.push_back (expPerLevel[i] + diff);
	}
	expPerLevel.pop_back();//last value is broken
}

void CHeroHandler::loadObstacles()
{
	auto loadObstacles = [](const JsonNode &node, bool absolute, std::map<int, CObstacleInfo> &out)
	{
		for(const JsonNode &obs : node.Vector())
		{
			int ID = obs["id"].Float();
			CObstacleInfo & obi = out[ID];
			obi.ID = ID;
			obi.defName = obs["defname"].String();
			obi.width = obs["width"].Float();
			obi.height = obs["height"].Float();
			obi.allowedTerrains = obs["allowedTerrain"].convertTo<std::vector<ETerrainType> >();
			obi.allowedSpecialBfields = obs["specialBattlefields"].convertTo<std::vector<BFieldType> >();
			obi.blockedTiles = obs["blockedTiles"].convertTo<std::vector<si16> >();
			obi.isAbsoluteObstacle = absolute;
		}
	};

	const JsonNode config(ResourceID("config/obstacles.json"));
	loadObstacles(config["obstacles"], false, obstacles);
	loadObstacles(config["absoluteObstacles"], true, absoluteObstacles);
	//loadObstacles(config["moats"], true, moats);
}

/// convert h3-style ID (e.g. Gobin Wolf Rider) to vcmi (e.g. goblinWolfRider)
static std::string genRefName(std::string input)
{
	boost::algorithm::replace_all(input, " ", ""); //remove spaces
	input[0] = std::tolower(input[0]); // to camelCase
	return input;
}

void CHeroHandler::loadBallistics()
{
	CLegacyConfigParser ballParser("DATA/BALLIST.TXT");

	ballParser.endLine(); //header
	ballParser.endLine();

	do
	{
		ballParser.readString();
		ballParser.readString();

		CHeroHandler::SBallisticsLevelInfo bli;
		bli.keep   = ballParser.readNumber();
		bli.tower  = ballParser.readNumber();
		bli.gate   = ballParser.readNumber();
		bli.wall   = ballParser.readNumber();
		bli.shots  = ballParser.readNumber();
		bli.noDmg  = ballParser.readNumber();
		bli.oneDmg = ballParser.readNumber();
		bli.twoDmg = ballParser.readNumber();
		bli.sum    = ballParser.readNumber();
		ballistics.push_back(bli);

		assert(bli.noDmg + bli.oneDmg + bli.twoDmg == 100 && bli.sum == 100);
	}
	while (ballParser.endLine());
}

std::vector<JsonNode> CHeroHandler::loadLegacyData(size_t dataSize)
{
	heroes.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser specParser("DATA/HEROSPEC.TXT");
	CLegacyConfigParser bioParser("DATA/HEROBIOS.TXT");
	CLegacyConfigParser parser("DATA/HOTRAITS.TXT");

	parser.endLine(); //ignore header
	parser.endLine();

	specParser.endLine(); //ignore header
	specParser.endLine();

	for (int i=0; i<GameConstants::HEROES_QUANTITY; i++)
	{
		JsonNode heroData;

		heroData["texts"]["name"].String() = parser.readString();
		heroData["texts"]["biography"].String() = bioParser.readString();
		heroData["texts"]["specialty"]["name"].String() = specParser.readString();
		heroData["texts"]["specialty"]["tooltip"].String() = specParser.readString();
		heroData["texts"]["specialty"]["description"].String() = specParser.readString();

		for(int x=0;x<3;x++)
		{
			JsonNode armySlot;
			armySlot["min"].Float() = parser.readNumber();
			armySlot["max"].Float() = parser.readNumber();
			armySlot["creature"].String() = genRefName(parser.readString());

			heroData["army"].Vector().push_back(armySlot);
		}
		parser.endLine();
		specParser.endLine();
		bioParser.endLine();

		h3Data.push_back(heroData);
	}
	return h3Data;
}

void CHeroHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));
	object->ID = HeroTypeID(heroes.size());
	object->imageIndex = heroes.size() + GameConstants::HERO_PORTRAIT_SHIFT; // 2 special frames + some extra portraits

	heroes.push_back(object);

	VLC->modh->identifiers.registerObject(scope, "hero", name, object->ID.getNum());
}

void CHeroHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(data, normalizeIdentifier(scope, "core", name));
	object->ID = HeroTypeID(index);
	object->imageIndex = index;

	assert(heroes[index] == nullptr); // ensure that this id was not loaded before
	heroes[index] = object;

	VLC->modh->identifiers.registerObject(scope, "hero", name, object->ID.getNum());
}

void CHeroHandler::afterLoadFinalization()
{
	for(ConstTransitivePtr<CHero> hero : heroes)
	{
		for(auto bonus : hero->specialty)
		{
			bonus->sid = hero->ID.getNum();
		}

		if(hero->specDeprecated.size() > 0 || hero->specialtyDeprecated.size() > 0)
		{
			logMod->debug("Converting specialty format for hero %s(%s)", hero->identifier, FactionID::encode(hero->heroClass->faction));
			std::vector<std::shared_ptr<Bonus>> convertedBonuses;
			for(const SSpecialtyInfo & spec : hero->specDeprecated)
			{
				for(std::shared_ptr<Bonus> b : SpecialtyInfoToBonuses(spec, hero->ID.getNum()))
					convertedBonuses.push_back(b);
			}
			for(const SSpecialtyBonus & spec : hero->specialtyDeprecated)
			{
				for(std::shared_ptr<Bonus> b : SpecialtyBonusToBonuses(spec, hero->ID.getNum()))
					convertedBonuses.push_back(b);
			}
			hero->specDeprecated.clear();
			hero->specialtyDeprecated.clear();
			// store and create json for logging
			std::vector<JsonNode> specVec;
			std::vector<std::string> specNames;
			for(std::shared_ptr<Bonus> bonus : convertedBonuses)
			{
				hero->specialty.push_back(bonus);
				specVec.push_back(bonus->toJsonNode());
				// find fitting & unique bonus name
				std::string bonusName = bonus->nameForBonus();
				if(vstd::contains(specNames, bonusName))
				{
					int suffix = 2;
					while(vstd::contains(specNames, bonusName + std::to_string(suffix)))
						suffix++;
					bonusName += std::to_string(suffix);
				}
				specNames.push_back(bonusName);
			}
			// log new format for easy copy-and-paste
			JsonNode specNode(JsonNode::JsonType::DATA_STRUCT);
			if(specVec.size() > 1)
			{
				JsonNode base = JsonUtils::intersect(specVec);
				if(base.containsBaseData())
				{
					specNode["base"] = base;
					for(JsonNode & node : specVec)
						node = JsonUtils::difference(node, base);
				}
			}
			// add json for bonuses
			specNode["bonuses"].Struct();
			for(int i = 0; i < specVec.size(); i++)
				specNode["bonuses"][specNames[i]] = specVec[i];
			logMod->trace("\"specialty\" : %s", specNode.toJson(true));
		}
	}
}

const Entity * CHeroHandler::getBaseByIndex(const int32_t index) const
{
	return getByIndex(index);
}

const HeroType * CHeroHandler::getByIndex(const int32_t index) const
{
	if(index < 0 || index >= heroes.size())
	{
		logGlobal->error("Unable to get hero with ID %d", int32_t(index));
		return nullptr;
	}
	else
	{
		return heroes.at(index).get();
	}
}

const HeroType * CHeroHandler::getById(const HeroTypeID & heroTypeID) const
{
	return getByIndex(heroTypeID.getNum());
}

void CHeroHandler::forEachBase(const std::function<void(const Entity *, bool &)>& cb) const
{
	bool stop = false;

	for(auto & object : heroes)
	{
		cb(object.get(), stop);
		if(stop)
			break;
	}
}

void CHeroHandler::forEach(const std::function<void(const HeroType *, bool &)>& cb) const
{
	bool stop = false;

	for(auto & object : heroes)
	{
		cb(object.get(), stop);
		if(stop)
			break;
	}
}

ui32 CHeroHandler::level (ui64 experience) const
{
	return boost::range::upper_bound(expPerLevel, experience) - std::begin(expPerLevel);
}

ui64 CHeroHandler::reqExp (ui32 level) const
{
	if(!level)
		return 0;

	if (level <= expPerLevel.size())
	{
		return expPerLevel[level-1];
	}
	else
	{
		logGlobal->warn("A hero has reached unsupported amount of experience");
		return expPerLevel[expPerLevel.size()-1];
	}
}

void CHeroHandler::loadTerrains()
{
	const JsonNode config(ResourceID("config/terrains.json"));

	terrCosts.reserve(GameConstants::TERRAIN_TYPES);
	for(const std::string & name : GameConstants::TERRAIN_NAMES)
		terrCosts.push_back(config[name]["moveCost"].Float());
}

std::vector<bool> CHeroHandler::getDefaultAllowed() const
{
	// Look Data/HOTRAITS.txt for reference
	std::vector<bool> allowedHeroes;
	allowedHeroes.reserve(heroes.size());

	for(const CHero * hero : heroes)
	{
		allowedHeroes.push_back(!hero->special);
	}

	return allowedHeroes;
}
