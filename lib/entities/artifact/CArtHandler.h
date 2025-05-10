/*
 * CArtHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifact.h"
#include "EArtifactClass.h"
#include "IHandlerBase.h"

#include <vcmi/Artifact.h>
#include <vcmi/ArtifactService.h>

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CArtHandler : public CHandlerBase<ArtifactID, Artifact, CArtifact, ArtifactService>
{
public:
	void addBonuses(CArtifact * art, const JsonNode & bonusList);

	static EArtifactClass stringToClass(const std::string & className); //TODO: rework EartClass to make this a constructor

	bool legalArtifact(const ArtifactID & id) const;
	static void makeItCreatureArt(CArtifact * a, bool onlyCreature = true);
	static void makeItCommanderArt(CArtifact * a, bool onlyCommander = true);

	~CArtHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	std::set<ArtifactID> getDefaultAllowed() const;

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CArtifact> loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) override;

private:
	void addSlot(CArtifact * art, const std::string & slotID) const;
	void loadSlots(CArtifact * art, const JsonNode & node) const;
	void loadClass(CArtifact * art, const JsonNode & node) const;
	void loadType(CArtifact * art, const JsonNode & node) const;
	void loadComponents(CArtifact * art, const JsonNode & node);
};

VCMI_LIB_NAMESPACE_END
