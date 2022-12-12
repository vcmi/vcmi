/*
* InterfaceBuilder.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"

#include "InterfaceObjectConfigurable.h"

#include "../CGameInfo.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CGeneralTextHandler.h"


InterfaceObjectConfigurable::InterfaceObjectConfigurable(const JsonNode & config):
	CIntObject()
{
	init(config);
}

InterfaceObjectConfigurable::InterfaceObjectConfigurable():
	CIntObject()
{
}

void InterfaceObjectConfigurable::addCallback(const std::string & callbackName, std::function<void(int)> callback)
{
	callbacks[callbackName] = callback;
}

void InterfaceObjectConfigurable::init(const JsonNode &config)
{
	OBJ_CONSTRUCTION;
	int unnamedObjectId = 0;
	const std::string unnamedObjectPrefix = "__widget_";
	
	for(const auto & item : config["items"].Vector())
	{
		std::string name = item["name"].isNull()
						? unnamedObjectPrefix + std::to_string(unnamedObjectId++)
						: item["name"].String();
		widgets[name] = buildWidget(item);
	}
}

std::string InterfaceObjectConfigurable::buildText(const JsonNode & config) const
{
	if(config.isNull())
		return "";
	
	if(config.isNumber())
	{
		return CGI->generaltexth->allTexts[config.Integer()];
	}
	return config.String();
}

std::shared_ptr<CIntObject> InterfaceObjectConfigurable::buildWidget(const JsonNode & config)
{
	assert(!config.isNull());
	auto type = config["type"].String();
	
	int x = 0, y = 0;
	if(!config["position"].isNull())
	{
		x = config["position"]["x"].Integer();
		y = config["position"]["y"].Integer();
	}
	
	std::string image;
	std::string text = buildText(config["text"]);
	auto alignment = EAlignment::CENTER;
	auto color = Colors::DEFAULT_KEY_COLOR;
	auto font = EFonts::FONT_TIMES;
	
	if(!config["image"].isNull())
		image = config["image"].String();
	if(!config["alignment"].isNull())
	{
		if(config["alignment"].String() == "left")
			alignment = EAlignment::TOPLEFT;
		if(config["alignment"].String() == "center")
			alignment = EAlignment::CENTER;
		if(config["alignment"].String() == "right")
			alignment = EAlignment::BOTTOMRIGHT;
	}
	if(!config["color"].isNull())
	{
		if(config["color"].String() == "yellow")
			color = Colors::YELLOW;
		if(config["color"].String() == "white")
			color = Colors::WHITE;
		if(config["color"].String() == "gold")
			color = Colors::METALLIC_GOLD;
		if(config["color"].String() == "green")
			color = Colors::GREEN;
		if(config["color"].String() == "orange")
			color = Colors::ORANGE;
		if(config["color"].String() == "bright-yellow")
			color = Colors::BRIGHT_YELLOW;
	}
	if(!config["font"].isNull())
	{
		if(config["font"].String() == "big")
			font = EFonts::FONT_BIG;
		if(config["font"].String() == "medium")
			font = EFonts::FONT_MEDIUM;
		if(config["font"].String() == "small")
			font = EFonts::FONT_SMALL;
		if(config["font"].String() == "tiny")
			font = EFonts::FONT_TINY;
	}
	
	
	if(type == "picture")
	{
		return std::make_shared<CPicture>(image, x, y);
	}
	if(type == "label")
	{
		return std::make_shared<CLabel>(x, y, font, alignment, color, text);
	}
	if(type == "toggleGroup")
	{
		auto group = std::make_shared<CToggleGroup>(0);
		group->pos.x += x;
		group->pos.y += y;
		if(!config["items"].isNull())
		{
			SObjectConstruction obj__i(group.get());
			int itemIdx = -1;
			for(const auto & item : config["items"].Vector())
			{
				itemIdx = item["index"].isNull() ? itemIdx + 1 : item["index"].Integer();
				group->addToggle(itemIdx, std::dynamic_pointer_cast<CToggleBase>(buildWidget(item)));
			}
		}
		if(!config["selected"].isNull())
			group->setSelected(config["selected"].Integer());
		if(!config["callback"].isNull())
			group->addCallback(callbacks[config["callback"].String()]);
		return group;
	}
	if(type == "toggleButton")
	{
		std::pair<std::string, std::string> zelp;
		if(!config["zelp"].isNull())
			zelp = CGI->generaltexth->zelp[config["zelp"].Integer()];
		auto button = std::make_shared<CToggleButton>(Point(x, y), image, zelp);
		if(!config["selected"].isNull())
			button->setSelected(config["selected"].Bool());
		if(!config["imageOrder"].isNull())
		{
			auto imgOrder = config["imageOrder"].Vector();
			assert(imgOrder.size() >= 4);
			button->setImageOrder(imgOrder[0].Integer(), imgOrder[1].Integer(), imgOrder[2].Integer(), imgOrder[3].Integer());
		}
		if(!config["callback"].isNull())
			button->addCallback(callbacks[config["callback"].String()]);
		return button;
	}
	if(type == "button")
	{
		std::pair<std::string, std::string> zelp;
		if(!config["zelp"].isNull())
			zelp = CGI->generaltexth->zelp[config["zelp"].Integer()];
		auto button = std::make_shared<CButton>(Point(x, y), image, zelp);
		if(!config["items"].isNull())
		{
			for(const auto & item : config["items"].Vector())
			{
				button->addOverlay(buildWidget(item));
			}
		}
		if(!config["callback"].isNull())
			button->addCallback(std::bind(callbacks[config["callback"].String()], 0));
		return button;
	}
	if(type == "labelGroup")
	{
		auto group = std::make_shared<CLabelGroup>(font, alignment, color);
		if(!config["items"].isNull())
		{
			for(const auto & item : config["items"].Vector())
			{
				if(!item["position"].isNull())
				{
					x = item["position"]["x"].Integer();
					y = item["position"]["y"].Integer();
				}
				if(!item["text"].isNull())
					text = buildText(item["text"]);
				group->add(x, y, text);
			}
		}
		return group;
	}
	return std::shared_ptr<CIntObject>(nullptr);
}
