/*
 * KeyBindingsWindow.h, part of VCMI engine
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
#include "../../../lib/texts/MetaString.h"

class CFilledTexture;
class CButton;
class CLabel;
class TransparentFilledRectangle;
class CSlider;
class CTextBox;

const int MAX_LINES = 11;
const int LINE_HEIGHT = 30;

class KeyBindingElement : public CIntObject
{
private:
	std::shared_ptr<CButton> buttonEdit;
	std::shared_ptr<CLabel> labelName;
	std::shared_ptr<CLabel> labelKeys;
	std::shared_ptr<TransparentFilledRectangle> seperationLine; // rectangle is cleaner than line...

	MetaString popupText;

	std::function<void(const std::string & id, const std::string & keyName)> func;

	void showPopupWindow(const Point & cursorPosition) override;
public:
	KeyBindingElement(std::string id, JsonNode keys, int elem, std::function<void(const std::string & id, const std::string & keyName)> func);
	KeyBindingElement(std::string group, int elem);
};

class KeyBindingsWindow : public CWindowObject
{
private:
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CLabel> labelTitle;
	std::shared_ptr<TransparentFilledRectangle> backgroundRect;
	std::shared_ptr<CSlider> slider;
	std::vector<std::shared_ptr<KeyBindingElement>> listElements;
	std::shared_ptr<CButton> buttonReset;

	void fillList(int start);
	void setKeyBinding(const std::string & id, const std::string & group, const std::string & keyName, bool append);
	void resetKeyBinding();

public:
	KeyBindingsWindow();
};

class KeyBindingsEditWindow : public CWindowObject
{
private:
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CTextBox> text;

	std::string id;
	std::function<void(const std::string & id, const std::string & keyName)> func;

	void keyReleased(const std::string & keyName) override;
	void notFocusedClick() override;
public:
	KeyBindingsEditWindow(const std::string & id, std::function<void(const std::string & id, const std::string & keyName)> func);
};

