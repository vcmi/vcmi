/*
 * CGDwelling.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CArmedInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGDwelling;

class DLL_LINKAGE CSpecObjInfo
{
public:
	CSpecObjInfo();
	virtual ~CSpecObjInfo() = default;

	virtual void serializeJson(JsonSerializeFormat & handler) = 0;

	const CGDwelling * owner;
};

class DLL_LINKAGE CCreGenAsCastleInfo : public virtual CSpecObjInfo
{
public:
	bool asCastle = false;
	ui32 identifier = 0;//h3m internal identifier

	std::vector<bool> allowedFactions;

	std::string instanceId;//vcmi map instance identifier
	void serializeJson(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CCreGenLeveledInfo : public virtual CSpecObjInfo
{
public:
	ui8 minLevel = 0;
	ui8 maxLevel = 7; //minimal and maximal level of creature in dwelling: <1, 7>

	void serializeJson(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CCreGenLeveledCastleInfo : public CCreGenAsCastleInfo, public CCreGenLeveledInfo
{
public:
	CCreGenLeveledCastleInfo() = default;
	void serializeJson(JsonSerializeFormat & handler) override;
};


class DLL_LINKAGE CGDwelling : public CArmedInstance
{
public:
	typedef std::vector<std::pair<ui32, std::vector<CreatureID> > > TCreaturesSet;

	CSpecObjInfo * info; //random dwelling options; not serialized
	TCreaturesSet creatures; //creatures[level] -> <vector of alternative ids (base creature and upgrades, creatures amount>

	CGDwelling();
	~CGDwelling() override;

	void initRandomObjectInfo();
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;

private:
	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void newTurn(CRandomGenerator & rand) const override;
	void setPropertyDer(ui8 what, ui32 val) override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void updateGuards() const;
	void heroAcceptsCreatures(const CGHeroInstance *h) const;

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & creatures;
	}
};


VCMI_LIB_NAMESPACE_END
