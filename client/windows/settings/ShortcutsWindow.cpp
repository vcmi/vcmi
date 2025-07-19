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

	shortcuts = JsonUtils::assembleFromFiles("config/shortcutsConfig");

	int count = 0;
	for(auto & group : shortcuts.Struct())
	{
		count++;
		count += group.second.Struct().size();
	}

	slider = std::make_shared<CSlider>(Point(backgroundRect->pos.x - pos.x + backgroundRect->pos.w - 18, backgroundRect->pos.y - pos.y + 1), backgroundRect->pos.h - 3, [this](int pos){ fillList(pos); redraw(); }, MAX_LINES, count, 0, Orientation::VERTICAL, CSlider::BROWN);
	slider->setPanningStep(LINE_HEIGHT);
	slider->setScrollBounds(Rect(-backgroundRect->pos.w + slider->pos.w, 0, slider->pos.x - pos.x + slider->pos.w, slider->pos.h));

	buttonReset = std::make_shared<CButton>(Point(411, 403), AnimationPath::builtin("settingsWindow/button80"), std::make_pair("", MetaString::createFromTextID("vcmi.shortcuts.reset").toString()));
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
		for(auto group = shortcuts.Struct().rbegin(); group != shortcuts.Struct().rend(); ++group)
		{
			if(i >= start)
				listElements.push_back(std::make_shared<ShortcutElement>(group->first, listElements.size()));
			i++;
			if(listElements.size() == MAX_LINES)
				return;
			for(auto & elem : group->second.Struct())
			{
				if(i >= start)
					listElements.push_back(std::make_shared<ShortcutElement>(elem.first, elem.second, listElements.size(), [this](const std::string & id, const std::string & keyName){
						auto str = MetaString::createFromTextID("vcmi.shortcuts.inputSet");
						str.replaceTextID("vcmi.shortcuts.shortcut." + id);
						str.replaceRawString(keyName);

						GAME->interface()->showYesNoDialog(str.toString(), [this, id, keyName](){
							setKeyBinding(id, keyName, true);
						}, [this, id, keyName](){
							setKeyBinding(id, keyName, false);
						});
					}));
				i++;
				if(listElements.size() == MAX_LINES)
					return;
			}
		}
	}();
}

void ShortcutsWindow::setKeyBinding(const std::string & id, const std::string & keyName, bool append)
{
	// TODO
	std::cout << id << "   " << keyName << "   " << append << "\n";

	fillList(slider->getValue());
}

void ShortcutsWindow::resetKeyBinding()
{
	// TODO

	fillList(slider->getValue());
}

ShortcutElement::ShortcutElement(std::string id, JsonNode keys, int elem, std::function<void(const std::string & id, const std::string & keyName)> func)
	: func(func)
{
	OBJECT_CONSTRUCTION;

	pos.x += 14;
	pos.y += 56;
	pos.y += elem * LINE_HEIGHT;

	std::string keyBinding = "";
	if(keys.isString())
		keyBinding = keys.String();
	else if(keys.isVector())
	{
		std::vector<std::string> strings;
		std::transform(keys.Vector().begin(), keys.Vector().end(), std::back_inserter(strings), [](const auto& k) { return k.String(); });
		keyBinding = boost::join(strings, " {gray||} ");
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

void ShortcutsEditWindow::keyPressed(const std::string & keyName)
{
	if(boost::algorithm::ends_with(keyName, "Ctrl") || boost::algorithm::ends_with(keyName, "Shift") || boost::algorithm::ends_with(keyName, "Alt")) // skip if only control key pressed
		return;
	close();
	func(id, keyName);
}
