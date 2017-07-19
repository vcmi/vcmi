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
#include "HeroBonus.h"


class JsonNode;

typedef Bonus::BonusType BonusTypeID;

class MacroString
{
	struct Item
	{
		enum ItemType
		{
			STRING,
			MACRO
		};
		Item(ItemType _type, std::string _value)
			: type(_type), value(_value){};
		ItemType type;
		std::string value; //constant string or macro name
	};
	std::vector<Item> items;

public:
	typedef std::function<std::string(const std::string &)> GetValue;

	MacroString(){};
	MacroString(const std::string & format);

	std::string build(const GetValue & getValue) const;
};

class DLL_LINKAGE CBonusType
{
public:
	CBonusType();
	~CBonusType();


	template<typename Handler> void serialize(Handler & h, const int version)
	{
		h & icon & nameTemplate & descriptionTemplate & hidden;

		if(!h.saving)
			buildMacros();
	}

protected:

private:
	void buildMacros();
	MacroString name, description;

	friend class CBonusTypeHandler;
	std::string icon;
	std::string nameTemplate, descriptionTemplate;

	bool hidden;
};


class DLL_LINKAGE CBonusTypeHandler : public IBonusTypeHandler
{
public:
	CBonusTypeHandler();
	virtual ~CBonusTypeHandler();

	std::string bonusToString(const std::shared_ptr<Bonus> & bonus, const IBonusBearer * bearer, bool description) const override;
	std::string bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const override;

	void load();
	void load(const JsonNode & config);

	template<typename Handler> void serialize(Handler & h, const int version)
	{
		h & bonusTypes;
	}

private:

	void loadItem(const JsonNode & source, CBonusType & dest);

	std::vector<CBonusType> bonusTypes; //index = BonusTypeID

};
