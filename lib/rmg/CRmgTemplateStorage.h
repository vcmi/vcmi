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

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

/// The CJsonRmgTemplateLoader loads templates from a JSON file.
class DLL_LINKAGE CRmgTemplateStorage : public IHandlerBase
{
public:
	CRmgTemplateStorage() = default;
	
	std::vector<bool> getDefaultAllowed() const override;
	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void afterLoadFinalization() override;
	
	const CRmgTemplate* getTemplate(const std::string & templateName) const;
	std::vector<const CRmgTemplate *> getTemplates() const;

private:
	std::map<std::string, std::unique_ptr<CRmgTemplate>> templates;
};


VCMI_LIB_NAMESPACE_END
