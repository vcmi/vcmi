/*
 * CSkillHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/HeroBonus.h"
#include "GameConstants.h"
#include "IHandlerBase.h"

class CSkillHandler;
class CGHeroInstance;
class CMap;
class JsonSerializeFormat;

class DLL_LINKAGE CSkill // secondary skill
{
protected:
    std::vector<CBonusSystemNode *> bonusByLevel; // bonuses provided by none, basic, advanced and expert level

public:
    CSkill();
    ~CSkill();

    void addNewBonus(const std::shared_ptr<Bonus>& b, int level);
    CBonusSystemNode * getBonus(int level);

    SecondarySkill id;
    std::string identifier;

    template <typename Handler> void serialize(Handler &h, const int version)
    {
        h & id & identifier;
        h & bonusByLevel;
    }

    friend class CSkillHandler;
};

class DLL_LINKAGE CSkillHandler: public CHandlerBase<SecondarySkill, CSkill>
{
public:
    CSkillHandler();
    virtual ~CSkillHandler();

    ///IHandler base
    std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
    void afterLoadFinalization() override;
    void beforeValidate(JsonNode & object) override;

    std::vector<bool> getDefaultAllowed() const override;
    const std::string getTypeName() const override;

    template <typename Handler> void serialize(Handler &h, const int version)
    {
        h & objects ;
    }

protected:
    CSkill * loadFromJson(const JsonNode & json, const std::string & identifier) override;
};
