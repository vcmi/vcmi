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
class CTextBox;

const int MAX_LINES = 11;
const int LINE_HEIGHT = 30;

class ShortcutElement : public CIntObject
{
private:
	std::shared_ptr<CButton> buttonEdit;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelKeys;
	std::shared_ptr<TransparentFilledRectangle> seperationLine; // rectangle is cleaner than line...

	std::function<void(const std::string & id, const std::string & keyName)> func;

public:
	ShortcutElement(std::string id, JsonNode keys, int elem, std::function<void(const std::string & id, const std::string & keyName)> func);
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
	std::shared_ptr<CButton> buttonReset;

	JsonNode shortcuts;

	void fillList(int start);
	void setKeyBinding(const std::string & id, const std::string & keyName, bool append);
	void resetKeyBinding();

public:
	ShortcutsWindow();
};

class ShortcutsEditWindow : public CWindowObject
{
private:
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CTextBox> text;

	std::string id;
	std::function<void(const std::string & id, const std::string & keyName)> func;

	void keyPressed(const std::string & keyName) override;
public:
	ShortcutsEditWindow(const std::string & id, std::function<void(const std::string & id, const std::string & keyName)> func);
};

