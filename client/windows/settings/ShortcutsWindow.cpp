/*
 * ShortcutsWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ShortcutsWindow.h"

#include "../../CPlayerInterface.h"
#include "../../GameEngine.h"
#include "../../GameInstance.h"
#include "../../gui/Shortcut.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/Slider.h"
#include "../../windows/InfoWindows.h"

#include "../../../lib/CConfigHandler.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/json/JsonUtils.h"

ShortcutsWindow::ShortcutsWindow()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;
	pos.w = 500;
	pos.h = 450;

	updateShadow();
	center();

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	buttonOk = std::make_shared<CButton>(Point(218, 404), AnimationPath::builtin("IOKAY"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT);
	labelTitle = std::make_shared<CLabel>(
		pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, MetaString::createFromTextID("vcmi.shortcuts.button.hover").toString()
	);
	backgroundRect = std::make_shared<TransparentFilledRectangle>(Rect(8, 48, pos.w - 16, 348), ColorRGBA(0, 0, 0, 64), ColorRGBA(128, 100, 75), 1);

	int count = 0;
	for(auto & group : shortcutsConfig.toJsonNode().Struct())
	{
		count++;
		count += group.second.Struct().size();
	}

	slider = std::make_shared<CSlider>(Point(backgroundRect->pos.x - pos.x + backgroundRect->pos.w - 18, backgroundRect->pos.y - pos.y + 1), backgroundRect->pos.h - 3, [this](int pos){ fillList(pos); redraw(); }, MAX_LINES, count, 0, Orientation::VERTICAL, CSlider::BROWN);
	slider->setPanningStep(LINE_HEIGHT);
	slider->setScrollBounds(Rect(-backgroundRect->pos.w + slider->pos.w, 0, slider->pos.x - pos.x + slider->pos.w, slider->pos.h));

	buttonReset = std::make_shared<CButton>(Point(411, 403), AnimationPath::builtin("settingsWindow/button80"), std::make_pair("", MetaString::createFromTextID("vcmi.shortcuts.reset.help").toString()));
	buttonReset->setOverlay(std::make_shared<CLabel>(0, 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, MetaString::createFromTextID("vcmi.shortcuts.reset").toString()));
	buttonReset->addCallback([this](){
		GAME->interface()->showYesNoDialog(MetaString::createFromTextID("vcmi.shortcuts.resetConfirm").toString(), [this](){
			resetKeyBinding();
		}, nullptr);
	});

	fillList(0);
}

void ShortcutsWindow::fillList(int start)
{
	OBJECT_CONSTRUCTION;

	listElements.clear();
	int i = 0;
	[&]{
		for(auto group = shortcutsConfig.toJsonNode().Struct().rbegin(); group != shortcutsConfig.toJsonNode().Struct().rend(); ++group)
		{
			if(i >= start)
				listElements.push_back(std::make_shared<ShortcutElement>(group->first, listElements.size()));
			i++;
			if(listElements.size() == MAX_LINES)
				return;
			for(auto & elem : group->second.Struct())
			{
				if(i >= start)
					listElements.push_back(std::make_shared<ShortcutElement>(elem.first, elem.second, listElements.size(), [this, group](const std::string & id, const std::string & keyName){
						auto str = MetaString::createFromTextID("vcmi.shortcuts.inputSet");
						str.replaceTextID("vcmi.shortcuts.shortcut." + id);
						str.replaceRawString(keyName);

						GAME->interface()->showYesNoDialog(str.toString(), [this, group, id, keyName](){
							setKeyBinding(id, group->first, keyName, true);
						}, [this, group, id, keyName](){
							setKeyBinding(id, group->first, keyName, false);
						});
					}));
				i++;
				if(listElements.size() == MAX_LINES)
					return;
			}
		}
	}();
}

void ShortcutsWindow::setKeyBinding(const std::string & id, const std::string & group, const std::string & keyName, bool append)
{
	auto existing = shortcutsConfig[group][id];
	Settings existingWrite = shortcutsConfig.write[group][id];
	if((existing.isVector() || (existing.isString() && !existing.String().empty())) && append)
	{
		JsonVector tmp;
		if(existing.isVector())
			tmp = existing.Vector();
		if(existing.isString())
			tmp.push_back(existing);
		if(!vstd::contains(tmp, JsonNode(keyName)))
			tmp.push_back(JsonNode(keyName));
		existingWrite->Vector() = tmp;
	}
	else
		existingWrite->String() = keyName;

	fillList(slider->getValue());
}

void ShortcutsWindow::resetKeyBinding()
{
	{
		Settings write = shortcutsConfig.write;
		write->clear();
	}
	{
		Settings write = shortcutsConfig.write;
		write->Struct() = JsonUtils::assembleFromFiles("config/shortcutsConfig.json").Struct();
	}

	fillList(slider->getValue());
}

ShortcutElement::ShortcutElement(std::string id, JsonNode keys, int elem, std::function<void(const std::string & id, const std::string & keyName)> func)
	: func(func)
{
	OBJECT_CONSTRUCTION;

	pos.x += 14;
	pos.y += 56;
	pos.y += elem * LINE_HEIGHT;

	pos.w = 455;
	pos.h = LINE_HEIGHT;

	addUsedEvents(SHOW_POPUP);

	popupText = MetaString::createFromTextID("vcmi.shortcuts.popup");
	popupText.replaceTextID("vcmi.shortcuts.shortcut." + id);

	std::string keyBinding = "";
	if(keys.isString())
	{
		keyBinding = keys.String();
		popupText.appendRawString(keyBinding);
	}
	else if(keys.isVector())
	{
		std::vector<std::string> strings;
		std::transform(keys.Vector().begin(), keys.Vector().end(), std::back_inserter(strings), [](const auto& k) { return k.String(); });
		keyBinding = boost::join(strings, " | ");
		popupText.appendRawString(boost::join(strings, "\n"));
	}

	labelName = std::make_shared<CLabel>(
		0, LINE_HEIGHT / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, MetaString::createFromTextID("vcmi.shortcuts.shortcut." + id).toString(), 245
	);
	labelKeys = std::make_shared<CLabel>(
		250, LINE_HEIGHT / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, keyBinding, 170
	);
	buttonEdit = std::make_shared<CButton>(Point(422, 3), AnimationPath::builtin("settingsWindow/button32"), std::make_pair("", MetaString::createFromTextID("vcmi.shortcuts.editButton.help").toString()));
	buttonEdit->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("settingsWindow/gear")));
	buttonEdit->addCallback([id, func](){
		ENGINE->windows().createAndPushWindow<ShortcutsEditWindow>(id, [func](const std::string & id, const std::string & keyName){
			if(func)
				func(id, keyName);
		});
	});
	if(elem < MAX_LINES - 1)
		seperationLine = std::make_shared<TransparentFilledRectangle>(Rect(0, LINE_HEIGHT, 456, 1), ColorRGBA(0, 0, 0, 64), ColorRGBA(128, 100, 75), 1);
}

ShortcutElement::ShortcutElement(std::string group, int elem)
	: func(nullptr)
{
	OBJECT_CONSTRUCTION;

	pos.x += 14;
	pos.y += 56;
	pos.y += elem * LINE_HEIGHT;

	labelName = std::make_shared<CLabel>(
		0, LINE_HEIGHT / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::YELLOW, MetaString::createFromTextID("vcmi.shortcuts.group." + group).toString(), 300
	);
	if(elem < MAX_LINES - 1)
		seperationLine = std::make_shared<TransparentFilledRectangle>(Rect(0, LINE_HEIGHT, 456, 1), ColorRGBA(0, 0, 0, 64), ColorRGBA(128, 100, 75), 1);
}

void ShortcutElement::showPopupWindow(const Point & cursorPosition)
{
	CRClickPopup::createAndPush(popupText.toString());
}

ShortcutsEditWindow::ShortcutsEditWindow(const std::string & id, std::function<void(const std::string & id, const std::string & keyName)> func)
	: CWindowObject(BORDERED)
	, id(id)
	, func(func)
{
	OBJECT_CONSTRUCTION;
	pos.w = 250;
	pos.h = 150;

	auto str = MetaString::createFromTextID("vcmi.shortcuts.input");
	str.replaceTextID("vcmi.shortcuts.shortcut." + id);

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	text = std::make_shared<CTextBox>(str.toString(), Rect(0, 0, 250, 150), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);

	updateShadow();
	center();

	addUsedEvents(KEY_NAME);
}

void ShortcutsEditWindow::keyReleased(const std::string & keyName)
{
	if(boost::algorithm::ends_with(keyName, "Ctrl") || boost::algorithm::ends_with(keyName, "Shift") || boost::algorithm::ends_with(keyName, "Alt")) // skip if only control key pressed
		return;
	close();
	func(id, keyName);
}
