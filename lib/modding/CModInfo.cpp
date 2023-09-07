/*
 * CModInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CModInfo.h"

#include "../CGeneralTextHandler.h"
#include "../VCMI_Lib.h"
#include "../filesystem/Filesystem.h"

VCMI_LIB_NAMESPACE_BEGIN

static JsonNode addMeta(JsonNode config, const std::string & meta)
{
	config.setMeta(meta);
	return config;
}

CModInfo::CModInfo():
	checksum(0),
	explicitlyEnabled(false),
	implicitlyEnabled(true),
	validation(PENDING)
{

}

CModInfo::CModInfo(const std::string & identifier, const JsonNode & local, const JsonNode & config):
	identifier(identifier),
	name(config["name"].String()),
	description(config["description"].String()),
	dependencies(config["depends"].convertTo<std::set<std::string>>()),
	conflicts(config["conflicts"].convertTo<std::set<std::string>>()),
	checksum(0),
	explicitlyEnabled(false),
	implicitlyEnabled(true),
	validation(PENDING),
	config(addMeta(config, identifier))
{
	version = CModVersion::fromString(config["version"].String());
	if(!config["compatibility"].isNull())
	{
		vcmiCompatibleMin = CModVersion::fromString(config["compatibility"]["min"].String());
		vcmiCompatibleMax = CModVersion::fromString(config["compatibility"]["max"].String());
	}

	if (!config["language"].isNull())
		baseLanguage = config["language"].String();
	else
		baseLanguage = "english";

	loadLocalData(local);
}

JsonNode CModInfo::saveLocalData() const
{
	std::ostringstream stream;
	stream << std::noshowbase << std::hex << std::setw(8) << std::setfill('0') << checksum;

	JsonNode conf;
	conf["active"].Bool() = explicitlyEnabled;
	conf["validated"].Bool() = validation != FAILED;
	conf["checksum"].String() = stream.str();
	return conf;
}

std::string CModInfo::getModDir(const std::string & name)
{
	return "MODS/" + boost::algorithm::replace_all_copy(name, ".", "/MODS/");
}

JsonPath CModInfo::getModFile(const std::string & name)
{
	return JsonPath::builtinTODO(getModDir(name) + "/mod.json");
}

void CModInfo::updateChecksum(ui32 newChecksum)
{
	// comment-out next line to force validation of all mods ignoring checksum
	if (newChecksum != checksum)
	{
		checksum = newChecksum;
		validation = PENDING;
	}
}

void CModInfo::loadLocalData(const JsonNode & data)
{
	bool validated = false;
	implicitlyEnabled = true;
	explicitlyEnabled = !config["keepDisabled"].Bool();
	checksum = 0;
	if (data.getType() == JsonNode::JsonType::DATA_BOOL)
	{
		explicitlyEnabled = data.Bool();
	}
	if (data.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		explicitlyEnabled = data["active"].Bool();
		validated = data["validated"].Bool();
		checksum  = strtol(data["checksum"].String().c_str(), nullptr, 16);
	}

	//check compatibility
	implicitlyEnabled &= (vcmiCompatibleMin.isNull() || CModVersion::GameVersion().compatible(vcmiCompatibleMin, true, true));
	implicitlyEnabled &= (vcmiCompatibleMax.isNull() || vcmiCompatibleMax.compatible(CModVersion::GameVersion(), true, true));

	if(!implicitlyEnabled)
		logGlobal->warn("Mod %s is incompatible with current version of VCMI and cannot be enabled", name);

	if (boost::iequals(config["modType"].String(), "translation")) // compatibility code - mods use "Translation" type at the moment
	{
		if (baseLanguage != VLC->generaltexth->getPreferredLanguage())
		{
			logGlobal->warn("Translation mod %s was not loaded: language mismatch!", name);
			implicitlyEnabled = false;
		}
	}

	if (isEnabled())
		validation = validated ? PASSED : PENDING;
	else
		validation = validated ? PASSED : FAILED;
}

bool CModInfo::checkModGameplayAffecting() const
{
	if (modGameplayAffecting.has_value())
		return *modGameplayAffecting;

	static const std::vector<std::string> keysToTest = {
		"heroClasses",
		"artifacts",
		"creatures",
		"factions",
		"objects",
		"heroes",
		"spells",
		"skills",
		"templates",
		"scripts",
		"battlefields",
		"terrains",
		"rivers",
		"roads",
		"obstacles"
	};

	JsonPath modFileResource(CModInfo::getModFile(identifier));

	if(CResourceHandler::get("initial")->existsResource(modFileResource))
	{
		const JsonNode modConfig(modFileResource);

		for(const auto & key : keysToTest)
		{
			if (!modConfig[key].isNull())
			{
				modGameplayAffecting = true;
				return *modGameplayAffecting;
			}
		}
	}
	modGameplayAffecting = false;
	return *modGameplayAffecting;
}

bool CModInfo::isEnabled() const
{
	return implicitlyEnabled && explicitlyEnabled;
}

void CModInfo::setEnabled(bool on)
{
	explicitlyEnabled = on;
}

VCMI_LIB_NAMESPACE_END
