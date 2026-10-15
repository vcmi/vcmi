/*
 * EditorTemplateView.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorTemplateView.h"

#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/TextAlignment.h"
#include "../../widgets/UiTheme.h"
#include "render/CAnimation.h"
#include "render/Canvas.h"
#include "render/Colors.h"
#include "render/EFont.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"

namespace
{
constexpr double ZOOM_STEP = 1.1;
constexpr int MIN_ZOOM_LEVEL = -40;
constexpr int MAX_ZOOM_LEVEL = 20;
constexpr double PINCH_STEP = 1.2;
constexpr int DETAILS_MIN_SIZE = 70;
constexpr int LINE_PICK_DISTANCE = 6;
constexpr int FIT_MARGIN = 20;

/// Resources shown on the cards, the number is the frame of the icon
const std::array<std::pair<GameResID, int>, 7> RESOURCES{{
	{GameResID::WOOD, 0}, {GameResID::MERCURY, 1}, {GameResID::ORE, 2}, {GameResID::SULFUR, 3}, {GameResID::CRYSTAL, 4}, {GameResID::GEMS, 5}, {GameResID::GOLD, 6}}};

const std::array<std::pair<ColorRGBA, ColorRGBA>, 8> PLAYER_COLORS{{
	{ColorRGBA(0xF8, 0x00, 0x00), ColorRGBA(0x92, 0x00, 0x00)},
	{ColorRGBA(0x00, 0x00, 0xF8), ColorRGBA(0x00, 0x00, 0x92)},
	{ColorRGBA(0x9B, 0x72, 0x51), ColorRGBA(0x35, 0x27, 0x1C)},
	{ColorRGBA(0x00, 0xFC, 0x00), ColorRGBA(0x00, 0x96, 0x00)},
	{ColorRGBA(0xF8, 0x80, 0x00), ColorRGBA(0x92, 0x4B, 0x00)},
	{ColorRGBA(0xF8, 0x00, 0xF8), ColorRGBA(0x92, 0x00, 0x92)},
	{ColorRGBA(0x00, 0xFC, 0xF8), ColorRGBA(0x00, 0x96, 0x94)},
	{ColorRGBA(0xC0, 0x78, 0x88), ColorRGBA(0x5A, 0x38, 0x40)}}};

bool needsBlackText(const ColorRGBA & background)
{
	const double luminance = (0.299 * background.r + 0.587 * background.g + 0.114 * background.b) / 255;
	return luminance > 0.5;
}

int swordCount(EMonsterStrength::EMonsterStrength strength)
{
	switch(strength)
	{
		case EMonsterStrength::ZONE_WEAK:
		case EMonsterStrength::GLOBAL_WEAK:
			return 1;
		case EMonsterStrength::ZONE_NORMAL:
		case EMonsterStrength::GLOBAL_NORMAL:
			return 2;
		case EMonsterStrength::ZONE_STRONG:
		case EMonsterStrength::GLOBAL_STRONG:
			return 3;
		default:
			return 0;
	}
}

/// Lengths of the drawn and the skipped parts of the line of a connection type
std::vector<int> linePattern(rmg::EConnectionType type)
{
	switch(type)
	{
		case rmg::EConnectionType::FICTIVE:
			return {12, 6};
		case rmg::EConnectionType::REPULSIVE:
			return {12, 5, 3, 5};
		case rmg::EConnectionType::WIDE:
			return {3, 5};
		case rmg::EConnectionType::FORCE_PORTAL:
			return {12, 5, 3, 5, 3, 5};
		default:
			return {};
	}
}

double distanceToSegment(const Point & point, const Point & from, const Point & to)
{
	const double dx = to.x - from.x;
	const double dy = to.y - from.y;
	const double lengthSquared = dx * dx + dy * dy;
	double t = lengthSquared > 0 ? ((point.x - from.x) * dx + (point.y - from.y) * dy) / lengthSquared : 0;
	t = std::clamp(t, 0.0, 1.0);
	return std::hypot(point.x - (from.x + t * dx), point.y - (from.y + t * dy));
}
}

EditorTemplateView::EditorTemplateView(const Rect & area)
	: CIntObject(LCLICK | DRAG | WHEEL | GESTURE)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	for(const auto & [resource, frame] : RESOURCES)
		resourceIcons.at(frame) = ENGINE->renderHandler().loadImage(AnimationPath::builtin("SMALRES"), frame, 0, EImageBlitMode::COLORKEY);
}

double EditorTemplateView::scale() const
{
	return std::pow(ZOOM_STEP, zoomLevel);
}

Point EditorTemplateView::toScreen(const Point & world) const
{
	const double factor = scale();
	return Point(static_cast<int>(std::lround((world.x - viewCenter.x) * factor)), static_cast<int>(std::lround((world.y - viewCenter.y) * factor))) + pos.dimensions() / 2;
}

Point EditorTemplateView::toWorld(const Point & screen) const
{
	const double factor = scale();
	const Point relative = screen - pos.dimensions() / 2;
	return Point(static_cast<int>(std::lround(relative.x / factor)), static_cast<int>(std::lround(relative.y / factor))) + viewCenter;
}

void EditorTemplateView::setTemplate(const std::shared_ptr<CRmgTemplate> & newTemplate)
{
	currentTemplate = newTemplate;
	selectedZone = -1;
	selectedConnection = -1;
	redraw();
}

Point EditorTemplateView::getViewCenter() const
{
	return viewCenter;
}

void EditorTemplateView::setSelectedZone(TRmgTemplateZoneId zone)
{
	selectedZone = zone;
	selectedConnection = -1;
	redraw();
}

void EditorTemplateView::setSelectedConnection(int connection)
{
	selectedConnection = connection;
	redraw();
}

void EditorTemplateView::zoom(int steps)
{
	zoomLevel = std::clamp(zoomLevel + steps, MIN_ZOOM_LEVEL, MAX_ZOOM_LEVEL);
	redraw();
}

void EditorTemplateView::resetZoom()
{
	zoomLevel = 0;
	redraw();
}

void EditorTemplateView::autoFit()
{
	if(!currentTemplate || currentTemplate->getZones().empty())
	{
		viewCenter = Point(0, 0);
		zoomLevel = 0;
		redraw();
		return;
	}

	Point minimum(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
	Point maximum(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
	for(const auto & [id, zone] : currentTemplate->getZones())
	{
		const int half = static_cast<int>(CARD_SIZE * zone->getVisibleSize() / 2);
		const Point center = zone->getVisiblePosition();
		minimum = Point(std::min(minimum.x, center.x - half), std::min(minimum.y, center.y - half));
		maximum = Point(std::max(maximum.x, center.x + half), std::max(maximum.y, center.y + half));
	}

	viewCenter = (minimum + maximum) / 2;

	const double width = std::max(1, maximum.x - minimum.x) + 2 * FIT_MARGIN;
	const double height = std::max(1, maximum.y - minimum.y) + 2 * FIT_MARGIN;
	const double fit = std::min(pos.w / width, pos.h / height);
	zoomLevel = std::clamp(static_cast<int>(std::floor(std::log(fit) / std::log(ZOOM_STEP))), MIN_ZOOM_LEVEL, MAX_ZOOM_LEVEL);
	redraw();
}

std::vector<EditorTemplateView::CardLayout> EditorTemplateView::cardLayouts() const
{
	std::vector<CardLayout> result;
	if(!currentTemplate)
		return result;

	for(const auto & [id, zone] : currentTemplate->getZones())
	{
		const double size = CARD_SIZE * zone->getVisibleSize() * scale();
		const Point center = toScreen(zone->getVisiblePosition());
		const int side = std::max(4, static_cast<int>(std::lround(size)));
		result.push_back({id, Rect(center - Point(side / 2, side / 2), Point(side, side))});
	}
	return result;
}

TRmgTemplateZoneId EditorTemplateView::cardAt(const Point & cursor) const
{
	const auto layouts = cardLayouts();
	for(auto it = layouts.rbegin(); it != layouts.rend(); ++it)
		if(it->area.isInside(cursor))
			return it->id;

	return -1;
}

int EditorTemplateView::connectionAt(const Point & cursor) const
{
	if(!currentTemplate)
		return -1;

	const auto layouts = cardLayouts();
	const auto centerOf = [&layouts](TRmgTemplateZoneId id, Point & center)
	{
		const auto found = std::ranges::find_if(layouts, [id](const CardLayout & layout) { return layout.id == id; });
		if(found == layouts.end())
			return false;

		center = found->area.center();
		return true;
	};

	const auto & connections = currentTemplate->getConnectedZoneIds();
	for(size_t i = 0; i < connections.size(); ++i)
	{
		Point from;
		Point to;
		if(centerOf(connections[i].getZoneA(), from) && centerOf(connections[i].getZoneB(), to) && distanceToSegment(cursor, from, to) <= LINE_PICK_DISTANCE)
			return static_cast<int>(i);
	}
	return -1;
}

void EditorTemplateView::select(TRmgTemplateZoneId zone, int connection)
{
	selectedZone = zone;
	selectedConnection = connection;

	if(connection >= 0)
	{
		if(onConnectionSelected)
			onConnectionSelected(connection);
	}
	else if(onZoneSelected)
		onZoneSelected(zone);

	redraw();
}

void EditorTemplateView::clickPressed(const Point & cursorPosition)
{
	const Point local = cursorPosition - pos.topLeft();
	const TRmgTemplateZoneId card = cardAt(local);

	draggingCard = false;
	panning = false;
	dragStart = local;

	if(card >= 0)
	{
		draggingCard = true;
		cardStart = currentTemplate->getZones().at(card)->getVisiblePosition();
		select(card, -1);
		return;
	}

	const int connection = connectionAt(local);
	if(connection >= 0)
	{
		select(-1, connection);
		return;
	}

	panning = true;
	select(-1, -1);
}

void EditorTemplateView::mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	if(draggingCard && selectedZone >= 0 && currentTemplate)
	{
		const double factor = scale();
		const Point local = cursorPosition - pos.topLeft();
		const Point delta(static_cast<int>(std::lround((local.x - dragStart.x) / factor)), static_cast<int>(std::lround((local.y - dragStart.y) / factor)));
		currentTemplate->getZones().at(selectedZone)->setVisiblePosition(cardStart + delta);

		if(onZoneMoved)
			onZoneMoved(selectedZone);

		redraw();
	}
	else if(panning)
	{
		const double factor = scale();
		viewCenter = viewCenter - Point(static_cast<int>(std::lround(lastUpdateDistance.x / factor)), static_cast<int>(std::lround(lastUpdateDistance.y / factor)));
		redraw();
	}
}

void EditorTemplateView::clickReleased(const Point & cursorPosition)
{
	if(draggingCard && selectedZone >= 0 && currentTemplate)
	{
		// the cards snap to the grid
		const auto zone = currentTemplate->getZones().at(selectedZone);
		const Point center = zone->getVisiblePosition();
		zone->setVisiblePosition(Point(GRID_SIZE * static_cast<int>(std::lround(double(center.x) / GRID_SIZE)), GRID_SIZE * static_cast<int>(std::lround(double(center.y) / GRID_SIZE))));

		if(onZoneMoved)
			onZoneMoved(selectedZone);

		// the pages of the window show the position
		if(onZoneSelected)
			onZoneSelected(selectedZone);
	}

	draggingCard = false;
	panning = false;
	redraw();
}

void EditorTemplateView::clickCancel(const Point & cursorPosition)
{
	draggingCard = false;
	panning = false;
}

void EditorTemplateView::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	const double factor = scale();
	viewCenter = viewCenter - Point(static_cast<int>(std::lround(lastUpdateDistance.x / factor)), static_cast<int>(std::lround(lastUpdateDistance.y / factor)));
	redraw();
}

void EditorTemplateView::gesturePinch(const Point & centerPosition, double lastUpdateFactor)
{
	pinchFactor *= lastUpdateFactor;

	if(pinchFactor > PINCH_STEP)
	{
		zoom(1);
		pinchFactor = 1.0;
	}
	else if(pinchFactor < 1.0 / PINCH_STEP)
	{
		zoom(-1);
		pinchFactor = 1.0;
	}
}

void EditorTemplateView::wheelScrolled(int distance)
{
	zoom(distance > 0 ? 1 : -1);
}

void EditorTemplateView::drawConnection(Canvas & to, const rmg::ZoneConnection & connection, bool selected) const
{
	const auto & zones = currentTemplate->getZones();
	if(!zones.count(connection.getZoneA()) || !zones.count(connection.getZoneB()))
		return;

	const Point from = toScreen(zones.at(connection.getZoneA())->getVisiblePosition()) + pos.topLeft();
	const Point target = toScreen(zones.at(connection.getZoneB())->getVisiblePosition()) + pos.topLeft();
	const ColorRGBA color = selected ? ColorRGBA(255, 220, 0) : ColorRGBA(0, 0, 0);

	const auto pattern = linePattern(connection.getConnectionType());
	const double length = std::hypot(double(target.x - from.x), double(target.y - from.y));

	const auto drawSegment = [&](double start, double end)
	{
		const double t0 = start / length;
		const double t1 = end / length;
		const Point a(from.x + static_cast<int>((target.x - from.x) * t0), from.y + static_cast<int>((target.y - from.y) * t0));
		const Point b(from.x + static_cast<int>((target.x - from.x) * t1), from.y + static_cast<int>((target.y - from.y) * t1));

		// the line is thick, so it is drawn a few times side by side
		for(int offset = -1; offset <= 1; ++offset)
			to.drawLine(a + Point(offset, 0), b + Point(offset, 0), color, color);
		for(int offset = -1; offset <= 1; offset += 2)
			to.drawLine(a + Point(0, offset), b + Point(0, offset), color, color);
	};

	if(length < 1)
		return;

	if(pattern.empty())
	{
		drawSegment(0, length);
	}
	else
	{
		double position = 0;
		size_t index = 0;
		while(position < length)
		{
			const double part = pattern[index % pattern.size()];
			if(index % 2 == 0)
				drawSegment(position, std::min(length, position + part));

			position += part;
			++index;
		}
	}

	const Point middle = (from + target) / 2;
	to.drawText(middle, EFonts::FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, std::to_string(connection.getGuardStrength()));
}

void EditorTemplateView::drawCard(Canvas & to, const CardLayout & layout) const
{
	const auto zone = currentTemplate->getZones().at(layout.id);
	const Rect area = layout.area + pos.topLeft();

	ColorRGBA fill(200, 200, 200);
	ColorRGBA edge(120, 120, 120);
	const auto type = zone->getType();
	if(type == ETemplateZoneType::PLAYER_START || type == ETemplateZoneType::CPU_START)
	{
		const auto & colors = PLAYER_COLORS.at(std::clamp(zone->getOwner().value_or(1) - 1, 0, 7));
		fill = colors.first;
		edge = colors.second;
	}
	else if(type == ETemplateZoneType::TREASURE)
	{
		fill = ColorRGBA(0xFA, 0xE5, 0x9D);
		edge = ColorRGBA(0xA5, 0x7D, 0x37);
	}
	else if(type == ETemplateZoneType::WATER)
	{
		fill = ColorRGBA(0x9D, 0xFA, 0xF7);
		edge = ColorRGBA(0x37, 0x44, 0xA5);
	}

	to.drawColor(area, fill);
	to.drawBorder(area, edge, std::max(2, area.w / 20));

	if(type == ETemplateZoneType::JUNCTION)
		to.drawBorderDashed(Rect(area.x + 4, area.y + 4, area.w - 8, area.h - 8), ColorRGBA(0, 0, 0));

	if(layout.id == selectedZone)
		to.drawBorder(Rect(area.x - 2, area.y - 2, area.w + 4, area.h + 4), ColorRGBA(255, 220, 0), 2);

	const ColorRGBA text = needsBlackText(fill) ? ColorRGBA(0, 0, 0) : ColorRGBA(255, 255, 255);
	to.drawText(area.topLeft() + Point(5, 3), EFonts::FONT_MEDIUM, text, ETextAlignment::TOPLEFT, std::to_string(layout.id));

	if(area.w < DETAILS_MIN_SIZE)
		return;

	const auto mines = zone->getMinesInfo();
	int row = 0;
	for(const auto & [resource, frame] : RESOURCES)
	{
		const auto found = mines.find(resource);
		if(found == mines.end() || found->second == 0)
			continue;

		const Point rowPosition = area.topLeft() + Point(5, 24 + row * 16);
		if(rowPosition.y + 16 > area.y + area.h)
			break;

		if(resourceIcons.at(frame))
			to.draw(resourceIcons.at(frame), rowPosition);
		to.drawText(rowPosition + Point(22, 8), EFonts::FONT_SMALL, text, ETextAlignment::CENTERLEFT, std::to_string(found->second));
		++row;
	}

	if(zone->getMaxTreasureValue())
		to.drawText(Point(area.x + area.w - 5, area.y + area.h - 18), EFonts::FONT_SMALL, text, ETextAlignment::BOTTOMRIGHT, std::to_string(zone->getMaxTreasureValue()));

	for(int sword = 0; sword < swordCount(zone->monsterStrength); ++sword)
		to.drawColor(Rect(area.x + 5 + sword * 12, area.y + area.h - 12, 9, 7), ColorRGBA(0x9E, 0x87, 0x45));
}

void EditorTemplateView::showAll(Canvas & to)
{
	CanvasClipRectGuard guard(to, pos);
	to.drawColor(pos, ColorRGBA(255, 255, 255));

	if(!currentTemplate)
		return;

	const auto & connections = currentTemplate->getConnectedZoneIds();
	for(size_t i = 0; i < connections.size(); ++i)
		drawConnection(to, connections[i], static_cast<int>(i) == selectedConnection);

	for(const auto & layout : cardLayouts())
		drawCard(to, layout);
}
