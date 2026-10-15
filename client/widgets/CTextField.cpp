/*
 * CTextField.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTextField.h"

#include "CTextInput.h"
#include "UiTheme.h"

#include "render/Canvas.h"

CTextField::CTextField(const Rect & area, const std::string & text, const std::function<void(const std::string &)> & callback)
	: callback(callback)
{
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;
	input = std::make_shared<CTextInput>(Rect(4, 0, area.w - 8, area.h), UiTheme::FONT, ETextAlignment::CENTERLEFT, false);
	input->setColor(UiTheme::TEXT);
	input->setText(text);
	input->setCallback([this](const std::string & changed)
	{
		if(this->callback)
			this->callback(changed);
	});
}

const std::string & CTextField::getText() const
{
	return input->getText();
}

void CTextField::setText(const std::string & text)
{
	input->setText(text);
	redraw();
}

void CTextField::setCallback(const std::function<void(const std::string &)> & newCallback)
{
	callback = newCallback;
}

void CTextField::setFilterFilename()
{
	input->setFilterFilename();
}

void CTextField::setFilterNumber(int minValue, int maxValue)
{
	input->setFilterNumber(minValue, maxValue);
}

void CTextField::setReadOnly(bool on)
{
	readOnly = on;
	input->setInputEnabled(!on);
	input->setColor(on ? UiTheme::TEXT_DISABLED : UiTheme::TEXT);
	redraw();
}

void CTextField::giveFocus()
{
	input->giveFocus();
}

void CTextField::showAll(Canvas & to)
{
	to.drawColorBlended(pos, UiTheme::FIELD_FILL);
	to.drawBorder(pos, input->hasFocus() ? UiTheme::BORDER_HIGHLIGHT : UiTheme::BORDER);

	CIntObject::showAll(to);
}
