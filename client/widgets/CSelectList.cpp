/*
 * CSelectList.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSelectList.h"

#include "Slider.h"
#include "UiTheme.h"

#include "../GameEngine.h"
#include "media/ISoundPlayer.h"
#include "render/Canvas.h"
#include "render/IImage.h"

namespace
{
constexpr int INDENT_WIDTH = 14;
constexpr int MARGIN = 4;
constexpr int BRANCH_WIDTH = 12;
constexpr int COLUMN_WIDTH = 40;
}

CSelectList::CSelectList(const Rect & area, Mode mode, int rowHeight)
	: CIntObject(LCLICK | DOUBLECLICK | SHOW_POPUP | WHEEL | MOVE | HOVER | GESTURE)
	, mode(mode)
	, rowHeight(rowHeight)
{
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;
	slider = std::make_shared<CSlider>(Point(pos.w - UiTheme::SLIDER_WIDTH, 0), pos.h, [this](int value) { onSliderMoved(value); },
		visibleRows(), 0, 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setScrollingEnabled(false);
	slider->disable();
}

CSelectList::~CSelectList() = default;

int CSelectList::visibleRows() const
{
	return std::max(1, pos.h / rowHeight);
}

Rect CSelectList::contentRect() const
{
	Rect result = pos;
	if(!slider->isDisabled())
		result.w -= UiTheme::SLIDER_WIDTH;
	return result;
}

Rect CSelectList::checkArea(int index) const
{
	const Row & row = rows.at(index);

	int x = pos.x + MARGIN + row.indent * INDENT_WIDTH;
	if(row.branch != Branch::NONE)
		x += BRANCH_WIDTH;

	const int y = pos.y + (index - firstRow) * rowHeight + rowHeight / 2 - UiTheme::CHECK_SIZE / 2;
	return Rect(x, y, UiTheme::CHECK_SIZE + UiTheme::SPACING, UiTheme::CHECK_SIZE);
}

Rect CSelectList::rightCheckArea(int index, int column) const
{
	const Rect content = contentRect();
	const int x = content.right() - MARGIN - (rightChecks - column) * COLUMN_WIDTH;
	const int y = pos.y + (index - firstRow) * rowHeight + rowHeight / 2 - UiTheme::CHECK_SIZE / 2;
	return Rect(x, y, COLUMN_WIDTH, UiTheme::CHECK_SIZE);
}

int CSelectList::rightCheckAt(int index, const Point & position) const
{
	for(int column = 0; column < rightChecks; ++column)
		if(rightCheckArea(index, column).isInside(position))
			return column;

	return -1;
}

int CSelectList::rowAt(const Point & position) const
{
	const Rect content = contentRect();
	if(!content.isInside(position))
		return -1;

	const int index = firstRow + (position.y - pos.y) / rowHeight;
	if(index < 0 || index >= static_cast<int>(rows.size()))
		return -1;

	return index;
}

void CSelectList::updateSlider()
{
	const int visible = visibleRows();
	const int total = static_cast<int>(rows.size());

	firstRow = std::clamp(firstRow, 0, std::max(0, total - visible));

	if(total > visible)
	{
		slider->setAmount(total);
		slider->scrollTo(firstRow, false);
		slider->enable();
	}
	else
	{
		slider->disable();
	}
}

void CSelectList::onSliderMoved(int value)
{
	firstRow = value;
	redraw();
}

void CSelectList::setRows(std::vector<Row> newRows, bool keepScroll)
{
	rows = std::move(newRows);
	hoveredRow = -1;

	if(!keepScroll)
		firstRow = 0;

	if(selectedRow >= static_cast<int>(rows.size()))
		selectedRow = -1;

	updateSlider();
	redraw();
}

const std::vector<CSelectList::Row> & CSelectList::getRows() const
{
	return rows;
}

size_t CSelectList::size() const
{
	return rows.size();
}

void CSelectList::setSelected(int index)
{
	selectedRow = index < static_cast<int>(rows.size()) ? index : -1;
	if(selectedRow >= 0)
		scrollTo(selectedRow);
	redraw();
}

int CSelectList::getSelected() const
{
	return selectedRow;
}

void CSelectList::setChecked(int index, bool on)
{
	if(index < 0 || index >= static_cast<int>(rows.size()))
		return;

	rows[index].checked = on;
	redraw();
}

bool CSelectList::isChecked(int index) const
{
	return index >= 0 && index < static_cast<int>(rows.size()) && rows[index].checked;
}

std::vector<int> CSelectList::getCheckedIndexes() const
{
	std::vector<int> result;
	for(int i = 0; i < static_cast<int>(rows.size()); ++i)
		if(rows[i].checked)
			result.push_back(i);
	return result;
}

void CSelectList::setAllChecked(bool on)
{
	for(auto & row : rows)
		row.checked = on && row.enabled && !row.separator;
	redraw();
}

void CSelectList::setRightChecks(int count)
{
	rightChecks = count;
	redraw();
}

void CSelectList::scrollTo(int index)
{
	const int visible = visibleRows();

	if(index < firstRow)
		firstRow = index;
	else if(index >= firstRow + visible)
		firstRow = index - visible + 1;

	updateSlider();
}

void CSelectList::scrollBy(int rowsCount)
{
	firstRow += rowsCount;
	updateSlider();
	redraw();
}

void CSelectList::clickPressed(const Point & cursorPosition)
{
	const int index = rowAt(cursorPosition);
	if(index < 0)
		return;

	Row & row = rows[index];
	if(row.separator || !row.enabled)
		return;

	ENGINE->sound().playSound(soundBase::button);

	if(row.branch != Branch::NONE)
	{
		const int markerLeft = pos.x + MARGIN + row.indent * INDENT_WIDTH;
		if(cursorPosition.x >= markerLeft && cursorPosition.x < markerLeft + BRANCH_WIDTH)
		{
			if(onBranch)
				onBranch(index);
			return;
		}
	}

	const int column = rightChecks > 0 ? rightCheckAt(index, cursorPosition) : -1;
	if(column >= 0)
	{
		bool & flag = column == 0 ? row.checked : row.checked2;
		flag = !flag;
		const bool checked = flag;
		redraw();

		if(column == 0 && onCheck)
			onCheck(index, checked);
		else if(column == 1 && onCheck2)
			onCheck2(index, checked);
		return;
	}

	const bool toggles = rightChecks == 0 && (mode == Mode::CHECK || (mode == Mode::CHECK_SELECT && checkArea(index).isInside(cursorPosition)));
	if(toggles)
	{
		row.checked = !row.checked;
		const bool checked = row.checked;
		redraw();

		if(onCheck)
			onCheck(index, checked);
		return;
	}

	selectedRow = index;
	redraw();

	if(onSelect)
		onSelect(index);
}

void CSelectList::clickDouble(const Point & cursorPosition)
{
	const int index = rowAt(cursorPosition);
	if(index < 0 || rows[index].separator || !rows[index].enabled)
		return;

	if(onActivate)
		onActivate(index);
}

void CSelectList::showPopupWindow(const Point & cursorPosition)
{
	const int index = rowAt(cursorPosition);
	if(index < 0)
		return;

	if(onPopup)
		onPopup(index);
}

void CSelectList::wheelScrolled(int distance)
{
	scrollBy(-distance * UiTheme::SCROLL_STEP);
}

void CSelectList::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	const int index = rowAt(cursorPosition);
	if(index == hoveredRow)
		return;

	hoveredRow = index;
	redraw();
}

void CSelectList::hover(bool on)
{
	if(!on && hoveredRow != -1)
	{
		hoveredRow = -1;
		redraw();
	}
}

void CSelectList::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	panningAccumulated = 0;
}

void CSelectList::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	panningAccumulated += lastUpdateDistance.y;

	const int steps = panningAccumulated / rowHeight;
	if(steps != 0)
	{
		panningAccumulated -= steps * rowHeight;
		scrollBy(-steps);
	}
}

void CSelectList::showAll(Canvas & to)
{
	to.drawColorBlended(pos, UiTheme::FIELD_FILL);
	to.drawBorder(pos, UiTheme::BORDER);

	const Rect content = contentRect();
	{
		CanvasClipRectGuard guard(to, content);

		const int last = std::min(static_cast<int>(rows.size()), firstRow + visibleRows() + 1);
		for(int i = firstRow; i < last; ++i)
		{
			const Row & row = rows[i];
			const Rect rowRect(content.x + 1, pos.y + (i - firstRow) * rowHeight, content.w - 2, rowHeight);

			if(row.separator)
			{
				to.drawColorBlended(Rect(rowRect.x + MARGIN, rowRect.y + rowRect.h / 2, rowRect.w - 2 * MARGIN, 1), UiTheme::SEPARATOR);
				continue;
			}

			if(i == selectedRow)
				to.drawColorBlended(rowRect, UiTheme::ROW_SELECTED);
			else if(i == hoveredRow && row.enabled)
				to.drawColorBlended(rowRect, UiTheme::ROW_HOVER);

			int x = rowRect.x + MARGIN + row.indent * INDENT_WIDTH;
			const int centerY = rowRect.y + rowRect.h / 2;

			ColorRGBA color = UiTheme::TEXT;
			if(!row.enabled)
				color = UiTheme::TEXT_DISABLED;
			else if(i == selectedRow && mode != Mode::CHECK)
				color = UiTheme::TEXT_SELECTED;

			if(row.branch != Branch::NONE)
			{
				to.drawText(Point(x + BRANCH_WIDTH / 2, centerY), UiTheme::FONT, UiTheme::TEXT_HINT, ETextAlignment::CENTER, row.branch == Branch::EXPANDED ? "-" : "+");
				x += BRANCH_WIDTH;
			}

			auto drawCheck = [&](const Rect & box, bool checked)
			{
				to.drawColorBlended(box, UiTheme::FIELD_FILL);
				to.drawBorder(box, row.enabled ? UiTheme::BORDER_HIGHLIGHT : UiTheme::TEXT_DISABLED);
				if(!checked)
					return;

				for(int line = 0; line < 2; ++line)
				{
					to.drawLine(Point(box.x + 3, box.y + 7 + line), Point(box.x + 6, box.y + 10 + line), color, color);
					to.drawLine(Point(box.x + 6, box.y + 10 + line), Point(box.x + 11, box.y + 3 + line), color, color);
				}
			};

			for(int column = 0; column < rightChecks; ++column)
			{
				const Rect area = rightCheckArea(i, column);
				drawCheck(Rect(area.x + (area.w - UiTheme::CHECK_SIZE) / 2, area.y, UiTheme::CHECK_SIZE, UiTheme::CHECK_SIZE), column == 0 ? row.checked : row.checked2);
			}

			if(mode != Mode::SINGLE && rightChecks == 0)
			{
				drawCheck(Rect(x, centerY - UiTheme::CHECK_SIZE / 2, UiTheme::CHECK_SIZE, UiTheme::CHECK_SIZE), row.checked);
				x += UiTheme::CHECK_SIZE + UiTheme::SPACING;
			}

			if(row.icon)
			{
				const Point iconSize = row.icon->dimensions();
				to.draw(row.icon, Point(x, rowRect.y + (rowRect.h - iconSize.y) / 2));
				x += iconSize.x + UiTheme::SPACING;
			}

			if(!row.detail.empty())
				to.drawText(Point(rowRect.right() - MARGIN - rightChecks * COLUMN_WIDTH, centerY), UiTheme::FONT, UiTheme::TEXT_HINT, ETextAlignment::CENTERRIGHT, row.detail);

			to.drawText(Point(x, centerY), UiTheme::FONT, color, ETextAlignment::CENTERLEFT, row.text);
		}
	}

	CIntObject::showAll(to);
}
