/*
 * CBonusTypeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "IBonusTypeHandler.h"
#include "IHandlerBase.h"
#include "bonuses/Bonus.h"

VCMI_LIB_NAMESPACE_BEGIN


class JsonNode;

using BonusTypeID = Bonus::BonusType;

class DLL_LINKAGE CBonusType
{
public:
	CBonusType();

	std::string getNameTextID() const;
	std::string getDescriptionTextID() const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & icon;
		h & identifier;
		h & hidden;

	}

private:
	friend class CBonusTypeHandler;

	std::string icon;
	std::string identifier;

	bool hidden;
};

class DLL_LINKAGE CBonusTypeHandler : public IBonusTypeHandler
{
public:
	CBonusTypeHandler();
	virtual ~CBonusTypeHandler();

	std::string bonusToString(const std::shared_ptr<Bonus> & bonus, const IBonusBearer * bearer, bool description) const override;
	std::string bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		//for now always use up to date configuration
		//once modded bonus type will be implemented, serialize only them
		std::vector<CBonusType> ignore;
		h & ignore;
	}
private:
	void load();
	void load(const JsonNode & config);
	void loadItem(const JsonNode & source, CBonusType & dest, const std::string & name) const;

	std::vector<CBonusType> bonusTypes; //index = BonusTypeID
};

VCMI_LIB_NAMESPACE_END

#ifndef INSTANTIATE_CBonusTypeHandler_HERE
extern template class std::vector<VCMI_LIB_WRAP_NAMESPACE(CBonusType)>;
#endif
