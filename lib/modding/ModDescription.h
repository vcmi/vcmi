/*
 * ModDescription.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct CModVersion;
struct ModVerificationInfo;
class JsonNode;

using TModID = std::string;
using TModList = std::vector<TModID>;
using TModSet = std::set<TModID>;

class DLL_LINKAGE ModDescription : boost::noncopyable
{
	TModID identifier;
	TModSet dependencies;
	TModSet softDependencies;
	TModSet conflicts;

	std::unique_ptr<JsonNode> config;

	TModSet loadModList(const JsonNode & configNode) const;

public:
	ModDescription(const TModID & fullID, const JsonNode & config);
	~ModDescription();

	const TModID & getID() const;
	TModID getParentID() const;

	const TModSet & getDependencies() const;
	const TModSet & getSoftDependencies() const;
	const TModSet & getConflicts() const;

	const std::string & getBaseLanguage() const;
	const std::string & getName() const;

	const JsonNode & getFilesystemConfig() const;
	const JsonNode & getConfig() const;

	CModVersion getVersion() const;
	ModVerificationInfo getVerificationInfo() const;

	bool affectsGameplay() const;
	bool isCompatibility() const;
	bool isTranslation() const;
	bool keepDisabled() const;
};

VCMI_LIB_NAMESPACE_END
