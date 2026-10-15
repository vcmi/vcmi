/*
 * CFormView.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CFormView.h"

#include "CCheckBox.h"
#include "CDropDown.h"
#include "CNumberInput.h"
#include "CTextButton.h"
#include "CTextField.h"
#include "TextControls.h"
#include "UiTheme.h"

#include "../GameEngine.h"
#include "../windows/CMessage.h"
#include "render/IFont.h"
#include "render/IRenderHandler.h"

namespace
{
constexpr int ROW_GAP = 4;
}

CFormView::CFormView(const Rect & area, int labelWidth)
	: CScrollPanel(area)
	, labelWidth(labelWidth)
{
}

Rect CFormView::controlRect(int height) const
{
	return Rect(labelWidth, rowTop, contentWidth() - labelWidth, height);
}

void CFormView::addLabel(const std::string & label, int height)
{
	if(label.empty() || labelWidth == 0)
		return;

	OBJECT_CONSTRUCTION_TARGETED(content());
	rows.push_back(std::make_shared<CLabel>(0, rowTop + height / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT, label, labelWidth - UiTheme::SPACING));
}

void CFormView::clear()
{
	for(auto & row : rows)
	{
		row->deactivate();
		content()->removeChild(row.get());
	}

	removedRows = std::move(rows);
	rows.clear();
	rowTop = 0;
	finish();
}

void CFormView::finish()
{
	setContentHeight(std::max(rowTop, 1));
}

std::shared_ptr<CTextField> CFormView::addText(const std::string & label, const std::string & value, const std::function<void(const std::string &)> & callback)
{
	addLabel(label, UiTheme::CONTROL_HEIGHT);

	OBJECT_CONSTRUCTION_TARGETED(content());
	auto result = std::make_shared<CTextField>(controlRect(UiTheme::CONTROL_HEIGHT), value, callback);
	rows.push_back(result);
	rowTop += UiTheme::CONTROL_HEIGHT + ROW_GAP;
	return result;
}

std::shared_ptr<CNumberInput> CFormView::addNumber(const std::string & label, int value, int minValue, int maxValue, const std::function<void(int)> & callback)
{
	addLabel(label, UiTheme::CONTROL_HEIGHT);

	OBJECT_CONSTRUCTION_TARGETED(content());
	auto result = std::make_shared<CNumberInput>(controlRect(UiTheme::CONTROL_HEIGHT), value, minValue, maxValue, callback);
	rows.push_back(result);
	rowTop += UiTheme::CONTROL_HEIGHT + ROW_GAP;
	return result;
}

std::shared_ptr<CCheckBox> CFormView::addCheck(const std::string & label, bool value, const std::function<void(bool)> & callback)
{
	addLabel(label, UiTheme::CONTROL_HEIGHT);

	OBJECT_CONSTRUCTION_TARGETED(content());
	auto result = std::make_shared<CCheckBox>(controlRect(UiTheme::CONTROL_HEIGHT), "", value, callback);
	rows.push_back(result);
	rowTop += UiTheme::CONTROL_HEIGHT + ROW_GAP;
	return result;
}

std::shared_ptr<CDropDown> CFormView::addChoice(const std::string & label, const std::vector<std::string> & options, int selected, const std::function<void(int)> & callback)
{
	addLabel(label, UiTheme::CONTROL_HEIGHT);

	OBJECT_CONSTRUCTION_TARGETED(content());
	auto result = std::make_shared<CDropDown>(controlRect(UiTheme::CONTROL_HEIGHT), options, selected, callback, label);
	rows.push_back(result);
	rowTop += UiTheme::CONTROL_HEIGHT + ROW_GAP;
	return result;
}

std::shared_ptr<CTextButton> CFormView::addButton(const std::string & label, const std::string & buttonText, const std::function<void()> & callback)
{
	addLabel(label, UiTheme::CONTROL_HEIGHT);

	OBJECT_CONSTRUCTION_TARGETED(content());
	auto result = std::make_shared<CTextButton>(controlRect(UiTheme::CONTROL_HEIGHT), buttonText, callback);
	rows.push_back(result);
	rowTop += UiTheme::CONTROL_HEIGHT + ROW_GAP;
	return result;
}

std::shared_ptr<CLabel> CFormView::addInfo(const std::string & label, const std::string & value)
{
	addLabel(label, UiTheme::CONTROL_HEIGHT);

	OBJECT_CONSTRUCTION_TARGETED(content());
	auto result = std::make_shared<CLabel>(labelWidth + 4, rowTop + UiTheme::CONTROL_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT_HINT, value, contentWidth() - labelWidth - 4);
	rows.push_back(result);
	rowTop += UiTheme::CONTROL_HEIGHT + ROW_GAP;
	return result;
}

std::shared_ptr<CLabel> CFormView::addHeading(const std::string & text)
{
	OBJECT_CONSTRUCTION_TARGETED(content());
	rowTop += ROW_GAP;
	auto result = std::make_shared<CLabel>(contentWidth() / 2, rowTop + UiTheme::CONTROL_HEIGHT / 2, UiTheme::FONT_TITLE, ETextAlignment::CENTER, UiTheme::TEXT_TITLE, text, contentWidth());
	rows.push_back(result);
	rowTop += UiTheme::CONTROL_HEIGHT + ROW_GAP;
	return result;
}

std::shared_ptr<CMultiLineLabel> CFormView::addParagraph(const std::string & text)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const auto font = ENGINE->renderHandler().loadFont(UiTheme::FONT);
	const int lines = static_cast<int>(CMessage::breakText(text, contentWidth(), UiTheme::FONT).size());
	const int height = std::max(lines * static_cast<int>(font->getLineHeight()), UiTheme::CONTROL_HEIGHT);

	auto result = std::make_shared<CMultiLineLabel>(Rect(0, rowTop, contentWidth(), height), UiTheme::FONT, ETextAlignment::TOPLEFT, UiTheme::TEXT, text);
	rows.push_back(result);
	rowTop += height + ROW_GAP;
	return result;
}

void CFormView::addWidget(const std::string & label, const std::shared_ptr<CIntObject> & widget)
{
	addLabel(label, widget->pos.h);

	content()->addChild(widget.get(), true);
	widget->moveBy(Point(labelWidth, rowTop));
	rows.push_back(widget);
	rowTop += widget->pos.h + ROW_GAP;
}

void CFormView::addSpace(int height)
{
	rowTop += height;
}
