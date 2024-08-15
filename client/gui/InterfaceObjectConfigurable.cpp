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
#include "../gui/ShortcutHandler.h"
#include "../gui/Shortcut.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../widgets/CComponent.h"
#include "../widgets/ComboBox.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/constants/StringConstants.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/filesystem/ResourcePath.h"

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
	REGISTER_BUILDER("multiLineLabel", &InterfaceObjectConfigurable::buildMultiLineLabel);
	REGISTER_BUILDER("toggleGroup", &InterfaceObjectConfigurable::buildToggleGroup);
	REGISTER_BUILDER("toggleButton", &InterfaceObjectConfigurable::buildToggleButton);
	REGISTER_BUILDER("button", &InterfaceObjectConfigurable::buildButton);
	REGISTER_BUILDER("labelGroup", &InterfaceObjectConfigurable::buildLabelGroup);
	REGISTER_BUILDER("slider", &InterfaceObjectConfigurable::buildSlider);
	REGISTER_BUILDER("layout", &InterfaceObjectConfigurable::buildLayout);
	REGISTER_BUILDER("comboBox", &InterfaceObjectConfigurable::buildComboBox);
	REGISTER_BUILDER("textInput", &InterfaceObjectConfigurable::buildTextInput);
	REGISTER_BUILDER("graphicalPrimitive", &InterfaceObjectConfigurable::buildGraphicalPrimitive);
	REGISTER_BUILDER("transparentFilledRectangle", &InterfaceObjectConfigurable::buildTransparentFilledRectangle);
	REGISTER_BUILDER("textBox", &InterfaceObjectConfigurable::buildTextBox);
}

void InterfaceObjectConfigurable::registerBuilder(const std::string & type, BuilderFunction f)
{
	builders[type] = f;
}

void InterfaceObjectConfigurable::addCallback(const std::string & callbackName, std::function<void(int)> callback)
{
	callbacks_int[callbackName] = callback;
}

void InterfaceObjectConfigurable::addCallback(const std::string & callbackName, std::function<void(std::string)> callback)
{
	callbacks_string[callbackName] = callback;
}


void InterfaceObjectConfigurable::deleteWidget(const std::string & name)
{
	auto iter = widgets.find(name);
	if(iter != widgets.end())
		widgets.erase(iter);
}

void InterfaceObjectConfigurable::loadCustomBuilders(const JsonNode & config)
{
	for(auto & item : config.Struct())
	{
		std::string typeName = item.first;
		JsonNode baseConfig = item.second;

		auto const & functor = [this, baseConfig](const JsonNode & widgetConfig) -> std::shared_ptr<CIntObject>
		{
			JsonNode actualConfig = widgetConfig;
			JsonUtils::mergeCopy(actualConfig, baseConfig);

			return this->buildWidget(actualConfig);
		};
		registerBuilder(typeName, functor);
	}
}

void InterfaceObjectConfigurable::build(const JsonNode &config)
{
	OBJECT_CONSTRUCTION;

	logGlobal->debug("Building configurable interface object");
	auto * items = &config;
	
	if(config.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		if (!config["library"].isNull())
		{
			if (config["library"].isString())
			{
				const JsonNode library(JsonPath::fromJson(config["library"]));
				loadCustomBuilders(library);
			}

			if (config["library"].isVector())
			{
				for (auto const & entry : config["library"].Vector())
				{
					const JsonNode library(JsonPath::fromJson(entry));
					loadCustomBuilders(library);
				}
			}
		}

		loadCustomBuilders(config["customTypes"]);

		for(auto & item : config["variables"].Struct())
		{
			logGlobal->debug("Read variable named %s", item.first);
			variables[item.first] = item.second;
		}

		items = &config["items"];
	}
	
	for(const auto & item : items->Vector())
		addWidget(item["name"].String(), buildWidget(item));

	// load only if set
	if (!config["width"].isNull())
		pos.w = config["width"].Integer();
	if (!config["height"].isNull())
		pos.h = config["height"].Integer();
}

void InterfaceObjectConfigurable::addConditional(const std::string & name, bool active)
{
	conditionals[name] = active;
}

void InterfaceObjectConfigurable::addWidget(const std::string & namePreferred, std::shared_ptr<CIntObject> widget)
{
	static const std::string unnamedObjectPrefix = "__widget_";

	std::string nameActual;

	if (widgets.count(namePreferred) == 0)
		nameActual = namePreferred;
	else
		logGlobal->error("Duplicated widget name: '%s'", namePreferred);

	if (nameActual.empty())
		nameActual = unnamedObjectPrefix + std::to_string(unnamedObjectId++);

	logGlobal->debug("Building widget with name %s", nameActual);
	widgets[nameActual] = widget;
}

std::string InterfaceObjectConfigurable::readText(const JsonNode & config) const
{
	if(config.isNull())
		return "";
	
	std::string s = config.String();
	if(s.empty())
		return s;
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
	logGlobal->debug("Unknown text alignment attribute");
	return ETextAlignment::CENTER;
}

ColorRGBA InterfaceObjectConfigurable::readColor(const JsonNode & config) const
{
	logGlobal->debug("Reading color");
	if(!config.isNull())
	{
		if(config.isString())
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
		if(config.isVector())
		{
			const auto & asVector = config.Vector();
			if(asVector.size() == 4)
				return ColorRGBA(asVector[0].Integer(), asVector[1].Integer(), asVector[2].Integer(), asVector[3].Integer());
			if(asVector.size() == 3)
				return ColorRGBA(asVector[0].Integer(), asVector[1].Integer(), asVector[2].Integer());
		}
	}
	logGlobal->debug("Unknown color attribute");
	return Colors::DEFAULT_KEY_COLOR;

}

PlayerColor InterfaceObjectConfigurable::readPlayerColor(const JsonNode & config) const
{
	logGlobal->debug("Reading PlayerColor");
	if(!config.isNull() && config.isString())
		return PlayerColor(PlayerColor::decode(config.String()));
	
	logGlobal->debug("Unknown PlayerColor attribute");
	return PlayerColor::CANNOT_DETERMINE;
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
		if(config.String() == "calisto")
			return EFonts::FONT_CALLI;
	}
	logGlobal->debug("Unknown font attribute");
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

EShortcut InterfaceObjectConfigurable::readHotkey(const JsonNode & config) const
{
	logGlobal->debug("Reading hotkey");

	if(config.getType() != JsonNode::JsonType::DATA_STRING)
	{
		logGlobal->error("Invalid hotket format in interface configuration! Expected string!", config.String());
		return EShortcut::NONE;
	}

	EShortcut result = GH.shortcuts().findShortcut(config.String());
	if (result == EShortcut::NONE)
		logGlobal->error("Invalid hotkey '%s' in interface configuration!", config.String());
	return result;
}

std::shared_ptr<CPicture> InterfaceObjectConfigurable::buildPicture(const JsonNode & config) const
{
	logGlobal->debug("Building widget CPicture");
	auto image = ImagePath::fromJson(config["image"]);
	auto position = readPosition(config["position"]);
	auto pic = std::make_shared<CPicture>(image, position.x, position.y);

	if ( config["playerColored"].Bool() && LOCPLINT)
		pic->setPlayerColor(LOCPLINT->playerID);
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
	auto maxWidth = config["maxWidth"].Integer();
	return std::make_shared<CLabel>(position.x, position.y, font, alignment, color, text, maxWidth);
}

std::shared_ptr<CMultiLineLabel> InterfaceObjectConfigurable::buildMultiLineLabel(const JsonNode & config) const
{	
	logGlobal->debug("Building widget CMultiLineLabel");
	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);
	auto text = readText(config["text"]);
	Rect rect = readRect(config["rect"]);
	if(!config["adoptHeight"].isNull() && config["adoptHeight"].Bool())
		rect.h = graphics->fonts[font]->getLineHeight() * 2;
	return std::make_shared<CMultiLineLabel>(rect, font, alignment, color, text);
}


std::shared_ptr<CToggleGroup> InterfaceObjectConfigurable::buildToggleGroup(const JsonNode & config) const
{
	logGlobal->debug("Building widget CToggleGroup");
	auto position = readPosition(config["position"]);
	auto group = std::make_shared<CToggleGroup>(0);
	group->pos += position;
	if(!config["items"].isNull())
	{
		OBJECT_CONSTRUCTION_TARGETED(group.get());
		int itemIdx = -1;
		for(const auto & item : config["items"].Vector())
		{
			itemIdx = item["index"].isNull() ? itemIdx + 1 : item["index"].Integer();
			auto newToggle = std::dynamic_pointer_cast<CToggleButton>(buildWidget(item));
			group->addToggle(itemIdx, newToggle);
		}
	}
	if(!config["selected"].isNull())
		group->setSelected(config["selected"].Integer());
	if(!config["callback"].isNull())
		group->addCallback(callbacks_int.at(config["callback"].String()));
	return group;
}

std::shared_ptr<CToggleButton> InterfaceObjectConfigurable::buildToggleButton(const JsonNode & config) const
{
	logGlobal->debug("Building widget CToggleButton");
	auto position = readPosition(config["position"]);
	auto image = AnimationPath::fromJson(config["image"]);
	auto help = readHintText(config["help"]);
	auto button = std::make_shared<CToggleButton>(position, image, help);
	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			button->setOverlay(buildWidget(item));
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
	loadToggleButtonCallback(button, config["callback"]);
	return button;
}

std::shared_ptr<CButton> InterfaceObjectConfigurable::buildButton(const JsonNode & config) const
{
	logGlobal->debug("Building widget CButton");
	auto position = readPosition(config["position"]);
	auto image = AnimationPath::fromJson(config["image"]);
	auto help = readHintText(config["help"]);
	auto button = std::make_shared<CButton>(position, image, help);
	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			button->setOverlay(buildWidget(item));
		}
	}
	if(!config["imageOrder"].isNull())
	{
		auto imgOrder = config["imageOrder"].Vector();
		assert(imgOrder.size() >= 4);
		button->setImageOrder(imgOrder[0].Integer(), imgOrder[1].Integer(), imgOrder[2].Integer(), imgOrder[3].Integer());
	}

	loadButtonBorderColor(button, config["borderColor"]);
	loadButtonCallback(button, config["callback"]);
	loadButtonHotkey(button, config["hotkey"]);
	return button;
}

void InterfaceObjectConfigurable::loadButtonBorderColor(std::shared_ptr<CButton> button, const JsonNode & config) const
{
	if (config.isNull())
		return;

	auto color = readColor(config);
	button->setBorderColor(color);
}

void InterfaceObjectConfigurable::loadToggleButtonCallback(std::shared_ptr<CToggleButton> button, const JsonNode & config) const
{
	if(config.isNull())
		return;

	std::string callbackName = config.String();

	if (callbacks_int.count(callbackName) > 0)
		button->addCallback(callbacks_int.at(callbackName));
	else
		logGlobal->error("Invalid callback '%s' in widget", callbackName );
}

void InterfaceObjectConfigurable::loadButtonCallback(std::shared_ptr<CButton> button, const JsonNode & config) const
{
	if(config.isNull())
		return;

	std::string callbackName = config.String();

	if (callbacks_int.count(callbackName) > 0)
		button->addCallback(std::bind(callbacks_int.at(callbackName), 0));
	else
		logGlobal->error("Invalid callback '%s' in widget", callbackName );
}

void InterfaceObjectConfigurable::loadButtonHotkey(std::shared_ptr<CButton> button, const JsonNode & config) const
{
	if(config.isNull())
		return;

	if(config.getType() != JsonNode::JsonType::DATA_STRING)
	{
		logGlobal->error("Invalid shortcut format - string expected!");
		return;
	}

	button->assignedKey = readHotkey(config);

	auto target = shortcuts.find(button->assignedKey);
	if (target == shortcuts.end())
		return;

	button->addCallback(target->second.callback);
	target->second.assignedButtons.push_back(button);
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
	auto value = config["selected"].Integer();
	bool horizontal = config["orientation"].String() == "horizontal";
	auto orientation = horizontal ? Orientation::HORIZONTAL : Orientation::VERTICAL;

	std::shared_ptr<CSlider> result;

	if (config["items"].isNull())
	{
		auto itemsVisible = config["itemsVisible"].Integer();
		auto itemsTotal = config["itemsTotal"].Integer();

		result = std::make_shared<CSlider>(position, length, callbacks_int.at(config["callback"].String()), itemsVisible, itemsTotal, value, orientation, style);
	}
	else
	{
		auto items = config["items"].convertTo<std::vector<int>>();
		result = std::make_shared<SliderNonlinear>(position, length, callbacks_int.at(config["callback"].String()), items, value, orientation, style);
	}


	if(!config["scrollBounds"].isNull())
	{
		Rect bounds = readRect(config["scrollBounds"]);
		result->setScrollBounds(bounds);
	}
	
	if(!config["panningStep"].isNull())
		result->setPanningStep(config["panningStep"].Integer());

	return result;
}

std::shared_ptr<CAnimImage> InterfaceObjectConfigurable::buildImage(const JsonNode & config) const
{
	logGlobal->debug("Building widget CAnimImage");
	auto position = readPosition(config["position"]);
	auto image = AnimationPath::fromJson(config["image"]);
	int group = config["group"].isNull() ? 0 : config["group"].Integer();
	int frame = config["frame"].isNull() ? 0 : config["frame"].Integer();
	return std::make_shared<CAnimImage>(image, frame, group, position.x, position.y);
}

std::shared_ptr<CFilledTexture> InterfaceObjectConfigurable::buildTexture(const JsonNode & config) const
{
	logGlobal->debug("Building widget CFilledTexture");
	auto image = ImagePath::fromJson(config["image"]);
	auto rect = readRect(config["rect"]);
	auto playerColor = readPlayerColor(config["color"]);
	if(playerColor.isValidPlayer())
	{
		auto result = std::make_shared<FilledTexturePlayerColored>(image, rect);
		result->setPlayerColor(playerColor);
		return result;
	}
	return std::make_shared<CFilledTexture>(image, rect);
}

std::shared_ptr<ComboBox> InterfaceObjectConfigurable::buildComboBox(const JsonNode & config)
{
	logGlobal->debug("Building widget ComboBox");
	auto position = readPosition(config["position"]);
	auto dropDownPosition = readPosition(config["dropDownPosition"]);
	auto image = AnimationPath::fromJson(config["image"]);
	auto help = readHintText(config["help"]);
	auto result = std::make_shared<ComboBox>(position, image, help, config["dropDown"], dropDownPosition);

	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			result->setOverlay(buildWidget(item));
		}
	}
	if(!config["imageOrder"].isNull())
	{
		auto imgOrder = config["imageOrder"].Vector();
		assert(imgOrder.size() >= 4);
		result->setImageOrder(imgOrder[0].Integer(), imgOrder[1].Integer(), imgOrder[2].Integer(), imgOrder[3].Integer());
	}

	loadButtonBorderColor(result, config["borderColor"]);
	loadButtonHotkey(result, config["hotkey"]);
	return result;
}

std::shared_ptr<CTextInput> InterfaceObjectConfigurable::buildTextInput(const JsonNode & config) const
{
	logGlobal->debug("Building widget CTextInput");
	auto rect = readRect(config["rect"]);
	auto offset = readPosition(config["backgroundOffset"]);
	auto bgName = ImagePath::fromJson(config["background"]);
	auto result = std::make_shared<CTextInput>(rect, offset, bgName);
	if(!config["alignment"].isNull())
		result->setAlignment(readTextAlignment(config["alignment"]));
	if(!config["font"].isNull())
		result->setFont(readFont(config["font"]));
	if(!config["color"].isNull())
		result->setColor(readColor(config["color"]));
	if(!config["text"].isNull() && config["text"].isString())
		result->setText(config["text"].String()); //for input field raw string is taken
	if(!config["callback"].isNull())
		result->setCallback(callbacks_string.at(config["callback"].String()));
	return result;
}

/// Small helper class that provides ownership for shared_ptr's of child elements
class InterfaceLayoutWidget : public CIntObject
{
public:
	std::vector<std::shared_ptr<CIntObject>> ownedChildren;
	InterfaceLayoutWidget();
};

InterfaceLayoutWidget::InterfaceLayoutWidget()
	:CIntObject() 
{
	setRedrawParent(true);
}

std::shared_ptr<CIntObject> InterfaceObjectConfigurable::buildLayout(const JsonNode & config)
{
	logGlobal->debug("Building widget Layout");
	bool vertical = config["vertical"].Bool();
	bool horizontal = config["horizontal"].Bool();
	bool dynamic = config["dynamic"].Bool();
	int distance = config["distance"].Integer();
	std::string customType = config["customType"].String();
	auto position = readPosition(config["position"]);

	auto result = std::make_shared<InterfaceLayoutWidget>();
	result->moveBy(position);
	Point layoutPosition;

	for(auto item : config["items"].Vector())
	{
		if (item["type"].String().empty())
			item["type"].String() = customType;

		if (!item["created"].isNull())
		{
			std::string name = item["created"].String();

			if (conditionals.count(name) != 0)
			{
				if (!conditionals.at(name))
					continue;
			}
			else
			{
				logMod->warn("Unknown condition %s in widget!", name);
			}
		}

		auto widget = buildWidget(item);

		addWidget(item["name"].String(), widget);
		result->ownedChildren.push_back(widget);
		result->addChild(widget.get(), false);

		widget->moveBy(position + layoutPosition);

		if (dynamic && vertical)
			layoutPosition.y += widget->pos.h;
		if (dynamic && horizontal)
			layoutPosition.x += widget->pos.w;

		if (vertical)
			layoutPosition.y += distance;
		if (horizontal)
			layoutPosition.x += distance;
	}

	return result;
}

std::shared_ptr<CShowableAnim> InterfaceObjectConfigurable::buildAnimation(const JsonNode & config) const
{
	logGlobal->debug("Building widget CShowableAnim");
	auto position = readPosition(config["position"]);
	auto image = AnimationPath::fromJson(config["image"]);
	ui8 flags = 0;
	if(!config["repeat"].Bool())
		flags |= CShowableAnim::EFlags::PLAY_ONCE;
	
	int group = config["group"].isNull() ? 0 : config["group"].Integer();
	auto anim = std::make_shared<CShowableAnim>(position.x, position.y, image, flags, 4, group);
	if(!config["alpha"].isNull())
		anim->setAlpha(config["alpha"].Integer());
	if(!config["callback"].isNull())
		anim->callback = std::bind(callbacks_int.at(config["callback"].String()), 0);
	if(!config["frames"].isNull())
	{
		auto b = config["frames"]["start"].Integer();
		auto e = config["frames"]["end"].Integer();
		anim->set(group, b, e);
	}
	return anim;
}

std::shared_ptr<CIntObject> InterfaceObjectConfigurable::buildGraphicalPrimitive(const JsonNode & config) const
{
	logGlobal->debug("Building widget GraphicalPrimitiveCanvas");

	auto rect = readRect(config["rect"]);
	auto widget = std::make_shared<GraphicalPrimitiveCanvas>(rect);

	for (auto const & entry : config["primitives"].Vector())
	{
		auto color = readColor(entry["color"]);
		auto typeString = entry["type"].String();
		auto pointA = readPosition(entry["a"]);
		auto pointB = readPosition(entry["b"]);

		if (typeString == "line")
			widget->addLine(pointA, pointB, color);
		if (typeString == "filledBox")
			widget->addBox(pointA, pointB, color);
		if (typeString == "rectangle")
			widget->addRectangle(pointA, pointB, color);
	}

	return widget;
}

std::shared_ptr<TransparentFilledRectangle> InterfaceObjectConfigurable::buildTransparentFilledRectangle(const JsonNode & config) const
{
	logGlobal->debug("Building widget TransparentFilledRectangle");

	auto rect = readRect(config["rect"]);
	auto color = readColor(config["color"]);
	if(!config["colorLine"].isNull())
	{
		auto colorLine = readColor(config["colorLine"]);
		return std::make_shared<TransparentFilledRectangle>(rect, color, colorLine);
	}
	return std::make_shared<TransparentFilledRectangle>(rect, color);
}

std::shared_ptr<CTextBox> InterfaceObjectConfigurable::buildTextBox(const JsonNode & config) const
{
	logGlobal->debug("Building widget CTextBox");

	auto rect = readRect(config["rect"]);
	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);
	auto text = readText(config["text"]);
	auto blueTheme = config["blueTheme"].Bool();

	return std::make_shared<CTextBox>(text, rect, blueTheme ? 1 : 0, font, alignment, color);
}

std::shared_ptr<CIntObject> InterfaceObjectConfigurable::buildWidget(JsonNode config) const
{
	assert(!config.isNull());
	logGlobal->debug("Building widget from config");
	//overrides from variables
	for(auto & item : config["overrides"].Struct())
	{
		logGlobal->debug("Config attribute %s was overridden by variable %s", item.first, item.second.String());
		config[item.first] = variables[item.second.String()];
	}
	
	auto type = config["type"].String();
	auto buildIterator = builders.find(type);
	if(buildIterator != builders.end())
		return (buildIterator->second)(config);

	logGlobal->error("Builder with type %s is not registered", type);
	return nullptr;
}

void InterfaceObjectConfigurable::setShortcutBlocked(EShortcut shortcut, bool isBlocked)
{
	auto target = shortcuts.find(shortcut);
	if (target == shortcuts.end())
		return;

	target->second.blocked = isBlocked;

	for	(auto & entry : widgets)
	{
		auto button = std::dynamic_pointer_cast<CButton>(entry.second);

		if (button && button->assignedKey == shortcut)
			button->block(isBlocked);
	}
}

void InterfaceObjectConfigurable::addShortcut(EShortcut shortcut, std::function<void()> callback)
{
	assert(shortcuts.count(shortcut) == 0);
	shortcuts[shortcut].callback = callback;
}

void InterfaceObjectConfigurable::keyPressed(EShortcut key)
{
	auto target = shortcuts.find(key);
	if (target == shortcuts.end())
		return;

	for (auto const & button :target->second.assignedButtons)
		if (button->isActive())
			return; // will be handled by button instance

	if (target->second.blocked)
		return;

	target->second.callback();
}
