/*
 * AObjectTypeHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "AObjectTypeHandler.h"

#include "IObjectInfo.h"
#include "../CGeneralTextHandler.h"
#include "../modding/IdentifierStorage.h"
#include "../VCMI_Lib.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../mapObjects/ObjectTemplate.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string AObjectTypeHandler::getJsonKey() const
{
	return modScope + ':' + subTypeName;
}

si32 AObjectTypeHandler::getIndex() const
{
	return type;
}

si32 AObjectTypeHandler::getSubIndex() const
{
	return subtype;
}

std::string AObjectTypeHandler::getTypeName() const
{
	return typeName;
}

std::string AObjectTypeHandler::getSubTypeName() const
{
	return subTypeName;
}

static ui32 loadJsonOrMax(const JsonNode & input)
{
	if (input.isNull())
		return std::numeric_limits<ui32>::max();
	else
		return static_cast<ui32>(input.Float());
}

void AObjectTypeHandler::init(const JsonNode & input)
{
	base = input["base"];

	if (!input["rmg"].isNull())
	{
		rmgInfo.value =     static_cast<ui32>(input["rmg"]["value"].Float());

		const JsonNode & mapLimit = input["rmg"]["mapLimit"];
		if (!mapLimit.isNull())
			rmgInfo.mapLimit = static_cast<ui32>(mapLimit.Float());

		rmgInfo.zoneLimit = loadJsonOrMax(input["rmg"]["zoneLimit"]);
		rmgInfo.rarity =    static_cast<ui32>(input["rmg"]["rarity"].Float());
	} // else block is not needed - set in constructor

	for (auto entry : input["templates"].Struct())
	{
		entry.second.setType(JsonNode::JsonType::DATA_STRUCT);
		JsonUtils::inherit(entry.second, base);

		auto * tmpl = new ObjectTemplate;
		tmpl->id = Obj(type);
		tmpl->subid = subtype;
		tmpl->stringID = entry.first; // FIXME: create "fullID" - type.object.template?
		tmpl->readJson(entry.second);
		templates.push_back(std::shared_ptr<const ObjectTemplate>(tmpl));
	}

	for(const JsonNode & node : input["sounds"]["ambient"].Vector())
		sounds.ambient.push_back(node.String());

	for(const JsonNode & node : input["sounds"]["visit"].Vector())
		sounds.visit.push_back(node.String());

	for(const JsonNode & node : input["sounds"]["removal"].Vector())
		sounds.removal.push_back(node.String());

	if(input["aiValue"].isNull())
		aiValue = std::nullopt;
	else
		aiValue = static_cast<std::optional<si32>>(input["aiValue"].Integer());

	battlefield = BattleField::NONE;

	if(!input["battleground"].isNull())
	{
		VLC->identifiers()->requestIdentifier("battlefield", input["battleground"], [this](int32_t identifier)
		{
			battlefield = BattleField(identifier);
		});
	}

	initTypeData(input);
}

bool AObjectTypeHandler::objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return false; // by default there are no overrides
}

void AObjectTypeHandler::preInitObject(CGObjectInstance * obj) const
{
	obj->ID = Obj(type);
	obj->subID = subtype;
	obj->typeName = typeName;
	obj->subTypeName = subTypeName;
}

void AObjectTypeHandler::initTypeData(const JsonNode & input)
{
	// empty implementation for overrides
}

bool AObjectTypeHandler::hasNameTextID() const
{
	return false;
}

std::string AObjectTypeHandler::getBaseTextID() const
{
	return TextIdentifier("mapObject", modScope, typeName, subTypeName).get();
}

std::string AObjectTypeHandler::getNameTextID() const
{
	return TextIdentifier(getBaseTextID(), "name").get();
}

std::string AObjectTypeHandler::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

SObjectSounds AObjectTypeHandler::getSounds() const
{
	return sounds;
}

void AObjectTypeHandler::clearTemplates()
{
	templates.clear();
}

void AObjectTypeHandler::addTemplate(const std::shared_ptr<const ObjectTemplate> & templ)
{
	templates.push_back(templ);
}

void AObjectTypeHandler::addTemplate(JsonNode config)
{
	config.setType(JsonNode::JsonType::DATA_STRUCT); // ensure that input is not null
	JsonUtils::inherit(config, base);
	auto * tmpl = new ObjectTemplate;
	tmpl->id = Obj(type);
	tmpl->subid = subtype;
	tmpl->stringID.clear(); // TODO?
	tmpl->readJson(config);
	templates.emplace_back(tmpl);
}

std::vector<std::shared_ptr<const ObjectTemplate>> AObjectTypeHandler::getTemplates() const
{
	return templates;
}

BattleField AObjectTypeHandler::getBattlefield() const
{
	return battlefield;
}

std::vector<std::shared_ptr<const ObjectTemplate>>AObjectTypeHandler::getTemplates(TerrainId terrainType) const
{
	std::vector<std::shared_ptr<const ObjectTemplate>> templates = getTemplates();
	std::vector<std::shared_ptr<const ObjectTemplate>> filtered;
	const auto cfun = [&](const std::shared_ptr<const ObjectTemplate> & obj)
	{
		return obj->canBePlacedAt(terrainType);
	};
	std::copy_if(templates.begin(), templates.end(), std::back_inserter(filtered), cfun);
	// H3 defines allowed terrains in a weird way - artifacts, monsters and resources have faulty masks here
	// Perhaps we should re-define faulty templates and remove this workaround (already done for resources)
	if (type == Obj::ARTIFACT || type == Obj::MONSTER)
		return templates;
	else
		return filtered;
}

std::shared_ptr<const ObjectTemplate> AObjectTypeHandler::getOverride(TerrainId terrainType, const CGObjectInstance * object) const
{
	std::vector<std::shared_ptr<const ObjectTemplate>> ret = getTemplates(terrainType);
	for (const auto & tmpl: ret)
	{
		if (objectFilter(object, tmpl))
			return tmpl;
	}
	return std::shared_ptr<const ObjectTemplate>(); //empty
}

const RandomMapInfo & AObjectTypeHandler::getRMGInfo()
{
	return rmgInfo;
}

std::optional<si32> AObjectTypeHandler::getAiValue() const
{
	return aiValue;
}

bool AObjectTypeHandler::isStaticObject()
{
	return false; // most of classes are not static
}

void AObjectTypeHandler::afterLoadFinalization()
{
}

std::unique_ptr<IObjectInfo> AObjectTypeHandler::getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return nullptr;
}

VCMI_LIB_NAMESPACE_END
