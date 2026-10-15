/*
 * CNumberInput.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CNumberInput.h"

#include "CTextButton.h"
#include "CTextField.h"
#include "UiTheme.h"

CNumberInput::CNumberInput(const Rect & area, int value, int minValue, int maxValue, const std::function<void(int)> & callback, int step)
	: callback(callback)
	, value(std::clamp(value, minValue, maxValue))
	, minValue(minValue)
	, maxValue(maxValue)
	, step(step)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;

	const int buttonWidth = area.h;

	decrease = std::make_shared<CTextButton>(Rect(0, 0, buttonWidth, area.h), "-", [this]() { applyValue(this->value - this->step); });
	field = std::make_shared<CTextField>(Rect(buttonWidth + 2, 0, area.w - 2 * buttonWidth - 4, area.h), std::to_string(this->value));
	increase = std::make_shared<CTextButton>(Rect(area.w - buttonWidth, 0, buttonWidth, area.h), "+", [this]() { applyValue(this->value + this->step); });

	if(minValue < maxValue)
		field->setFilterNumber(minValue, maxValue);

	field->setCallback([this](const std::string & text)
	{
		const int parsed = std::clamp(std::atoi(text.c_str()), this->minValue, this->maxValue);
		if(parsed == this->value)
			return;

		this->value = parsed;
		if(this->callback)
			this->callback(parsed);
	});
}

void CNumberInput::applyValue(int newValue)
{
	newValue = std::clamp(newValue, minValue, maxValue);
	if(newValue == value)
		return;

	value = newValue;
	field->setText(std::to_string(value));

	if(callback)
		callback(value);
}

int CNumberInput::getValue() const
{
	return value;
}

void CNumberInput::setValue(int newValue)
{
	value = std::clamp(newValue, minValue, maxValue);
	field->setText(std::to_string(value));
}

void CNumberInput::setCallback(const std::function<void(int)> & newCallback)
{
	callback = newCallback;
}

void CNumberInput::block(bool on)
{
	decrease->block(on);
	increase->block(on);
	field->setReadOnly(on);
}

void CNumberInput::setRange(int newMin, int newMax)
{
	minValue = newMin;
	maxValue = newMax;
	if(minValue < maxValue)
		field->setFilterNumber(minValue, maxValue);
	setValue(value);
}
