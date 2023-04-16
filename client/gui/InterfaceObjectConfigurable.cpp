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
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CGeneralTextHandler.h"

static std::map<std::string, int> KeycodeMap{
	{"up", SDLK_UP},
	{"down", SDLK_DOWN},
	{"left", SDLK_LEFT},
	{"right", SDLK_RIGHT},
	{"space", SDLK_SPACE},
	{"escape", SDLK_ESCAPE},
	{"backspace", SDLK_BACKSPACE},
	{"enter", SDLK_RETURN}
};


InterfaceObjectConfigurable::InterfaceObjectConfigurable(const JsonNode & config, int used, Point offset):
	InterfaceObjectConfigurable(used, offset)
{
	build(config);
}

InterfaceObjectConfigurable::InterfaceObjectConfigurable(int used, Point offset):
	CIntObject(used, offset)
{
	REGISTER_BUILDER("picture", &InterfaceObjectConfigurable::buildPicture);
	REGISTER_BUILDER("image", &InterfaceObjectConfigurable::buildImage);
	REGISTER_BUILDER("texture", &InterfaceObjectConfigurable::buildTexture);
	REGISTER_BUILDER("animation", &InterfaceObjectConfigurable::buildAnimation);
	REGISTER_BUILDER("label", &InterfaceObjectConfigurable::buildLabel);
	REGISTER_BUILDER("toggleGroup", &InterfaceObjectConfigurable::buildToggleGroup);
	REGISTER_BUILDER("toggleButton", &InterfaceObjectConfigurable::buildToggleButton);
	REGISTER_BUILDER("button", &InterfaceObjectConfigurable::buildButton);
	REGISTER_BUILDER("labelGroup", &InterfaceObjectConfigurable::buildLabelGroup);
	REGISTER_BUILDER("slider", &InterfaceObjectConfigurable::buildSlider);
}

void InterfaceObjectConfigurable::registerBuilder(const std::string & type, BuilderFunction f)
{
	builders[type] = f;
}

void InterfaceObjectConfigurable::addCallback(const std::string & callbackName, std::function<void(int)> callback)
{
	callbacks[callbackName] = callback;
}

void InterfaceObjectConfigurable::deleteWidget(const std::string & name)
{
	auto iter = widgets.find(name);
	if(iter != widgets.end())
		widgets.erase(iter);
}

void InterfaceObjectConfigurable::build(const JsonNode &config)
{
	OBJ_CONSTRUCTION;
	logGlobal->debug("Building configurable interface object");
	auto * items = &config;
	
	if(config.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		for(auto & item : config["variables"].Struct())
		{
			logGlobal->debug("Read variable named %s", item.first);
			variables[item.first] = item.second;
		}
		
		items = &config["items"];
	}
	
	const std::string unnamedObjectPrefix = "__widget_";
	for(const auto & item : items->Vector())
	{
		std::string name = item["name"].isNull()
						? unnamedObjectPrefix + std::to_string(unnamedObjectId++)
						: item["name"].String();
		logGlobal->debug("Building widget with name %s", name);
		widgets[name] = buildWidget(item);
	}
}

std::string InterfaceObjectConfigurable::readText(const JsonNode & config) const
{
	if(config.isNull())
		return "";
	
	std::string s = config.String();
	logGlobal->debug("Reading text from translations by key: %s", s);
	return CGI->generaltexth->translate(s);
}

Point InterfaceObjectConfigurable::readPosition(const JsonNode & config) const
{
	Point p;
	logGlobal->debug("Reading point");
	p.x = config["x"].Integer();
	p.y = config["y"].Integer();
	return p;
}

Rect InterfaceObjectConfigurable::readRect(const JsonNode & config) const
{
	Rect p;
	logGlobal->debug("Reading rect");
	p.x = config["x"].Integer();
	p.y = config["y"].Integer();
	p.w = config["w"].Integer();
	p.h = config["h"].Integer();
	return p;
}

ETextAlignment InterfaceObjectConfigurable::readTextAlignment(const JsonNode & config) const
{
	logGlobal->debug("Reading text alignment");
	if(!config.isNull())
	{
		if(config.String() == "center")
			return ETextAlignment::CENTER;
		if(config.String() == "left")
			return ETextAlignment::TOPLEFT;
		if(config.String() == "right")
			return ETextAlignment::BOTTOMRIGHT;
	}
	logGlobal->debug("Uknown text alignment attribute");
	return ETextAlignment::CENTER;
}

SDL_Color InterfaceObjectConfigurable::readColor(const JsonNode & config) const
{
	logGlobal->debug("Reading color");
	if(!config.isNull())
	{
		if(config.String() == "yellow")
			return Colors::YELLOW;
		if(config.String() == "white")
			return Colors::WHITE;
		if(config.String() == "gold")
			return Colors::METALLIC_GOLD;
		if(config.String() == "green")
			return Colors::GREEN;
		if(config.String() == "orange")
			return Colors::ORANGE;
		if(config.String() == "bright-yellow")
			return Colors::BRIGHT_YELLOW;
	}
	logGlobal->debug("Uknown color attribute");
	return Colors::DEFAULT_KEY_COLOR;
	
}
EFonts InterfaceObjectConfigurable::readFont(const JsonNode & config) const
{
	logGlobal->debug("Reading font");
	if(!config.isNull())
	{
		if(config.String() == "big")
			return EFonts::FONT_BIG;
		if(config.String() == "medium")
			return EFonts::FONT_MEDIUM;
		if(config.String() == "small")
			return EFonts::FONT_SMALL;
		if(config.String() == "tiny")
			return EFonts::FONT_TINY;
	}
	logGlobal->debug("Uknown font attribute");
	return EFonts::FONT_TIMES;
}

std::pair<std::string, std::string> InterfaceObjectConfigurable::readHintText(const JsonNode & config) const
{
	logGlobal->debug("Reading hint text");
	std::pair<std::string, std::string> result;
	if(!config.isNull())
	{
		if(config.getType() == JsonNode::JsonType::DATA_STRUCT)
		{
			result.first = readText(config["hover"]);
			result.second = readText(config["help"]);
			return result;
		}
		if(config.getType() == JsonNode::JsonType::DATA_STRING)
		{
			logGlobal->debug("Reading hint text (help) from generaltext handler:%sd", config.String());
			result.first  = CGI->generaltexth->translate( config.String(), "hover");
			result.second = CGI->generaltexth->translate( config.String(), "help");
		}
	}
	return result;
}

int InterfaceObjectConfigurable::readKeycode(const JsonNode & config) const
{
	logGlobal->debug("Reading keycode");
	if(config.getType() == JsonNode::JsonType::DATA_INTEGER)
		return config.Integer();
	
	if(config.getType() == JsonNode::JsonType::DATA_STRING)
	{
		auto s = config.String();
		if(s.size() == 1) //keyboard symbol
			return s[0];

		if (KeycodeMap.count(s))
			return KeycodeMap[s];

		logGlobal->error("Invalid keycode '%s' in interface configuration!", config.String());
		return SDLK_UNKNOWN;
	}

	logGlobal->error("Invalid keycode format in interface configuration! Expected string or integer!", config.String());
	return SDLK_UNKNOWN;
}

std::shared_ptr<CPicture> InterfaceObjectConfigurable::buildPicture(const JsonNode & config) const
{
	logGlobal->debug("Building widget CPicture");
	auto image = config["image"].String();
	auto position = readPosition(config["position"]);
	auto pic = std::make_shared<CPicture>(image, position.x, position.y);
	if(!config["visible"].isNull())
		pic->visible = config["visible"].Bool();

	if ( config["playerColored"].Bool() && LOCPLINT)
		pic->colorize(LOCPLINT->playerID);
	return pic;
}

std::shared_ptr<CLabel> InterfaceObjectConfigurable::buildLabel(const JsonNode & config) const
{
	logGlobal->debug("Building widget CLabel");
	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);
	auto text = readText(config["text"]);
	auto position = readPosition(config["position"]);
	return std::make_shared<CLabel>(position.x, position.y, font, alignment, color, text);
}

std::shared_ptr<CToggleGroup> InterfaceObjectConfigurable::buildToggleGroup(const JsonNode & config) const
{
	logGlobal->debug("Building widget CToggleGroup");
	auto position = readPosition(config["position"]);
	auto group = std::make_shared<CToggleGroup>(0);
	group->pos += position;
	if(!config["items"].isNull())
	{
		OBJ_CONSTRUCTION_TARGETED(group.get());
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
		group->addCallback(callbacks.at(config["callback"].String()));
	return group;
}

std::shared_ptr<CToggleButton> InterfaceObjectConfigurable::buildToggleButton(const JsonNode & config) const
{
	logGlobal->debug("Building widget CToggleButton");
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	auto help = readHintText(config["help"]);
	auto button = std::make_shared<CToggleButton>(position, image, help);
	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			button->addOverlay(buildWidget(item));
		}
	}
	if(!config["selected"].isNull())
		button->setSelected(config["selected"].Bool());
	if(!config["imageOrder"].isNull())
	{
		auto imgOrder = config["imageOrder"].Vector();
		assert(imgOrder.size() >= 4);
		button->setImageOrder(imgOrder[0].Integer(), imgOrder[1].Integer(), imgOrder[2].Integer(), imgOrder[3].Integer());
	}
	if(!config["callback"].isNull())
	{
		std::string callbackName = config["callback"].String();

		if (callbacks.count(callbackName))
			button->addCallback(callbacks.at(callbackName));
		else
			logGlobal->error("Invalid callback '%s' in widget", callbackName );
	}
	return button;
}

std::shared_ptr<CButton> InterfaceObjectConfigurable::buildButton(const JsonNode & config) const
{
	logGlobal->debug("Building widget CButton");
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	auto help = readHintText(config["help"]);
	auto button = std::make_shared<CButton>(position, image, help);
	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			button->addOverlay(buildWidget(item));
		}
	}
	if(!config["imageOrder"].isNull())
	{
		auto imgOrder = config["imageOrder"].Vector();
		assert(imgOrder.size() >= 4);
		button->setImageOrder(imgOrder[0].Integer(), imgOrder[1].Integer(), imgOrder[2].Integer(), imgOrder[3].Integer());
	}
	if(!config["callback"].isNull())
		button->addCallback(std::bind(callbacks.at(config["callback"].String()), 0));
	if(!config["hotkey"].isNull())
	{
		if(config["hotkey"].getType() == JsonNode::JsonType::DATA_VECTOR)
		{
			for(auto k : config["hotkey"].Vector())
				button->assignedKeys.insert(readKeycode(k));
		}
		else
			button->assignedKeys.insert(readKeycode(config["hotkey"]));
	}
	return button;
}

std::shared_ptr<CLabelGroup> InterfaceObjectConfigurable::buildLabelGroup(const JsonNode & config) const
{
	logGlobal->debug("Building widget CLabelGroup");
	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);
	auto group = std::make_shared<CLabelGroup>(font, alignment, color);
	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			auto position = readPosition(item["position"]);
			auto text = readText(item["text"]);
			group->add(position.x, position.y, text);
		}
	}
	return group;
}

std::shared_ptr<CSlider> InterfaceObjectConfigurable::buildSlider(const JsonNode & config) const
{
	logGlobal->debug("Building widget CSlider");
	auto position = readPosition(config["position"]);
	int length = config["size"].Integer();
	auto style = config["style"].String() == "brown" ? CSlider::BROWN : CSlider::BLUE;
	auto itemsVisible = config["itemsVisible"].Integer();
	auto itemsTotal = config["itemsTotal"].Integer();
	auto value = config["selected"].Integer();
	bool horizontal = config["orientation"].String() == "horizontal";
	auto const & result = std::make_shared<CSlider>(position, length, callbacks.at(config["callback"].String()), itemsVisible, itemsTotal, value, horizontal, style);

	if (!config["scrollBounds"].isNull())
	{
		Rect bounds = readRect(config["scrollBounds"]);
		result->setScrollBounds(bounds);
	}

	return result;
}

std::shared_ptr<CAnimImage> InterfaceObjectConfigurable::buildImage(const JsonNode & config) const
{
	logGlobal->debug("Building widget CAnimImage");
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	int group = config["group"].isNull() ? 0 : config["group"].Integer();
	int frame = config["frame"].isNull() ? 0 : config["frame"].Integer();
	return std::make_shared<CAnimImage>(image, frame, group, position.x, position.y);
}

std::shared_ptr<CFilledTexture> InterfaceObjectConfigurable::buildTexture(const JsonNode & config) const
{
	logGlobal->debug("Building widget CFilledTexture");
	auto image = config["image"].String();
	auto rect = readRect(config["rect"]);
	return std::make_shared<CFilledTexture>(image, rect);
}

std::shared_ptr<CShowableAnim> InterfaceObjectConfigurable::buildAnimation(const JsonNode & config) const
{
	logGlobal->debug("Building widget CShowableAnim");
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	ui8 flags = 0;
	if(!config["repeat"].Bool())
		flags |= CShowableAnim::EFlags::PLAY_ONCE;
	
	int group = config["group"].isNull() ? 0 : config["group"].Integer();
	auto anim = std::make_shared<CShowableAnim>(position.x, position.y, image, flags, 4, group);
	if(!config["alpha"].isNull())
		anim->setAlpha(config["alpha"].Integer());
	if(!config["callback"].isNull())
		anim->callback = std::bind(callbacks.at(config["callback"].String()), 0);
	if(!config["frames"].isNull())
	{
		auto b = config["frames"]["start"].Integer();
		auto e = config["frames"]["end"].Integer();
		anim->set(group, b, e);
	}
	return anim;
}

std::shared_ptr<CIntObject> InterfaceObjectConfigurable::buildWidget(JsonNode config) const
{
	assert(!config.isNull());
	logGlobal->debug("Building widget from config");
	//overrides from variables
	for(auto & item : config["overrides"].Struct())
	{
		logGlobal->debug("Config attribute %s was overriden by variable %s", item.first, item.second.String());
		config[item.first] = variables[item.second.String()];
	}
	
	auto type = config["type"].String();
	auto buildIterator = builders.find(type);
	if(buildIterator != builders.end())
		return (buildIterator->second)(config);

	logGlobal->error("Builder with type %s is not registered", type);
	return nullptr;
}
