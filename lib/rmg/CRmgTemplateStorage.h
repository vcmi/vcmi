/*
 * CRmgTemplateStorage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../IHandlerBase.h"
#include "../int3.h"
#include "CRmgTemplate.h"

class JsonNode;

/// The CJsonRmgTemplateLoader loads templates from a JSON file.
class DLL_LINKAGE CRmgTemplateStorage : public IHandlerBase
{
public:
	CRmgTemplateStorage() = default;
	
	std::vector<bool> getDefaultAllowed() const override;
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	
	const CRmgTemplate * getTemplate(const std::string & templateFullId) const;
	const CRmgTemplate * getTemplateByName(const std::string & templateName) const;
	std::vector<const CRmgTemplate *> getTemplates() const;

private:
	std::map<std::string, CRmgTemplate> templates; //FIXME: doesn't IHandlerBase cover this?
	std::map<std::string, CRmgTemplate> templatesByName;
};

