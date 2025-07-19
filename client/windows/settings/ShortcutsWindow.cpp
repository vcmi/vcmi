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

#include "../../GameEngine.h"
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
					listElements.push_back(std::make_shared<ShortcutElement>(elem.first, elem.second, listElements.size()));
				i++;
				if(listElements.size() == MAX_LINES)
					return;
			}
		}
	}();
}

ShortcutElement::ShortcutElement(std::string id, JsonNode keys, int elem)
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
		keyBinding = boost::join(strings, " | ");
	}

	labelName = std::make_shared<CLabel>(
		0, LINE_HEIGHT / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, MetaString::createFromTextID("vcmi.shortcuts.shortcut." + id).toString(), 245
	);
	labelKeys = std::make_shared<CLabel>(
		250, LINE_HEIGHT / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, keyBinding, 170
	);
	buttonEdit = std::make_shared<CButton>(Point(422, 3), AnimationPath::builtin("settingsWindow/button32"), std::make_pair("", MetaString::createFromTextID("vcmi.shortcuts.editButton.help").toString()));
	buttonEdit->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("settingsWindow/gear")));
	buttonEdit->addCallback([id](){
		ENGINE->windows().createAndPushWindow<ShortcutsEditWindow>(id);
	});
	if(elem < MAX_LINES - 1)
		seperationLine = std::make_shared<TransparentFilledRectangle>(Rect(0, LINE_HEIGHT, 456, 1), ColorRGBA(0, 0, 0, 64), ColorRGBA(128, 100, 75), 1);
}

ShortcutElement::ShortcutElement(std::string group, int elem)
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

ShortcutsEditWindow::ShortcutsEditWindow(const std::string & id)
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;
	pos.w = 200;
	pos.h = 100;

	updateShadow();
	center();

	addUsedEvents(KEY_NAME);
}

void ShortcutsEditWindow::keyPressed(const std::string & keyName)
{
	std::cout << keyName << "\n";
}
