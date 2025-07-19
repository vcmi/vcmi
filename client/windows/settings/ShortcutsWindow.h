/*
 * ShortcutsWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../CWindowObject.h"
#include "../../../lib/json/JsonNode.h"

class CFilledTexture;
class CButton;
class CLabel;
class TransparentFilledRectangle;
class CSlider;

const int MAX_LINES = 11;
const int LINE_HEIGHT = 30;

class ShortcutElement : public CIntObject
{
private:
	std::shared_ptr<CButton> buttonEdit;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelKeys;
	std::shared_ptr<TransparentFilledRectangle> seperationLine; // rectangle is cleaner than line...

public:
	ShortcutElement(std::string id, JsonNode keys, int elem);
	ShortcutElement(std::string group, int elem);
};

class ShortcutsWindow : public CWindowObject
{
private:
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<TransparentFilledRectangle> backgroundRect;
	std::shared_ptr<CSlider> slider;
	std::vector<std::shared_ptr<ShortcutElement>> listElements;

	JsonNode shortcuts;

	void fillList(int start);

public:
	ShortcutsWindow();
};

class ShortcutsEditWindow : public CWindowObject
{
private:
	void keyPressed(const std::string & keyName) override;
public:
	ShortcutsEditWindow(const std::string & id);
};

