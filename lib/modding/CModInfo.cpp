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
	config.setModScope(meta);
	return config;
}

std::set<TModID> CModInfo::readModList(const JsonNode & input)
{
	std::set<TModID> result;

	for (auto const & string : input.convertTo<std::set<std::string>>())
		result.insert(boost::to_lower_copy(string));

	return result;
}

CModInfo::CModInfo():
	explicitlyEnabled(false),
	implicitlyEnabled(true),
	validation(PENDING)
{

}

CModInfo::CModInfo(const std::string & identifier, const JsonNode & local, const JsonNode & config):
	identifier(identifier),
	dependencies(readModList(config["depends"])),
	conflicts(readModList(config["conflicts"])),
	explicitlyEnabled(false),
	implicitlyEnabled(true),
	validation(PENDING),
	config(addMeta(config, identifier))
{
	if (!config["name"].String().empty())
		verificationInfo.name = config["name"].String();
	else
		verificationInfo.name = identifier;

	verificationInfo.version = CModVersion::fromString(config["version"].String());
	verificationInfo.parent = identifier.substr(0, identifier.find_last_of('.'));
	if(verificationInfo.parent == identifier)
		verificationInfo.parent.clear();

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
	stream << std::noshowbase << std::hex << std::setw(8) << std::setfill('0') << verificationInfo.checksum;

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
	if (newChecksum != verificationInfo.checksum)
	{
		verificationInfo.checksum = newChecksum;
		validation = PENDING;
	}
}

void CModInfo::loadLocalData(const JsonNode & data)
{
	bool validated = false;
	implicitlyEnabled = true;
	explicitlyEnabled = !config["keepDisabled"].Bool();
	verificationInfo.checksum = 0;
	if (data.isStruct())
	{
		explicitlyEnabled = data["active"].Bool();
		validated = data["validated"].Bool();
		updateChecksum(strtol(data["checksum"].String().c_str(), nullptr, 16));
	}

	//check compatibility
	implicitlyEnabled &= (vcmiCompatibleMin.isNull() || CModVersion::GameVersion().compatible(vcmiCompatibleMin, true, true));
	implicitlyEnabled &= (vcmiCompatibleMax.isNull() || vcmiCompatibleMax.compatible(CModVersion::GameVersion(), true, true));

	if(!implicitlyEnabled)
		logGlobal->warn("Mod %s is incompatible with current version of VCMI and cannot be enabled", verificationInfo.name);

	if (config["modType"].String() == "Translation")
	{
		if (baseLanguage != CGeneralTextHandler::getPreferredLanguage())
		{
			if (identifier.find_last_of('.') == std::string::npos)
				logGlobal->warn("Translation mod %s was not loaded: language mismatch!", verificationInfo.name);
			implicitlyEnabled = false;
		}
	}
	if (config["modType"].String() == "Compatibility")
	{
		// compatibility mods are always explicitly enabled
		// however they may be implicitly disabled - if one of their dependencies is missing
		explicitlyEnabled = true;
	}

	if (isEnabled())
		validation = validated ? PASSED : PENDING;
	else
		validation = validated ? PASSED : FAILED;

	verificationInfo.impactsGameplay = checkModGameplayAffecting();
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

const ModVerificationInfo & CModInfo::getVerificationInfo() const
{
	assert(!verificationInfo.name.empty());
	return verificationInfo;
}

bool CModInfo::isEnabled() const
{
	return implicitlyEnabled && explicitlyEnabled;
}

VCMI_LIB_NAMESPACE_END
