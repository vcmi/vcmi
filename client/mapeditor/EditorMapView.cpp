/*
 * EditorMapView.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorMapView.h"

#include "EditorMapRendererContext.h"

#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "../mapView/MapRendererContextState.h"
#include "../mapView/MapViewCache.h"
#include "../mapView/MapViewModel.h"
#include "render/CAnimation.h"
#include "render/Canvas.h"
#include "render/Colors.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"

#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/TerrainTile.h"

namespace
{
constexpr int NATIVE_TILE = 32;
constexpr int MIN_TILE_SIZE = 8;
constexpr int MAX_TILE_SIZE = 96;
constexpr double SCROLL_SPEED = 0.6;
constexpr size_t MAX_GHOST_OBJECTS = 64;
constexpr uint8_t GHOST_ALPHA = 180;
constexpr double PINCH_STEP = 1.2;

const ColorRGBA GRID_COLOR(0, 0, 0, 120);
const ColorRGBA BLOCKED_COLOR(255, 0, 0, 64);
const ColorRGBA VISITABLE_COLOR(200, 200, 0, 64);
const ColorRGBA TILE_SELECTION_COLOR(128, 128, 128, 96);
const ColorRGBA PICKABLE_COLOR(255, 211, 0, 96);
const ColorRGBA RUBBER_BAND_FILL(255, 255, 255, 40);
}

EditorMapView::EditorMapView(const Rect & area, EditorMap & editor)
	: CIntObject(LCLICK | SHOW_POPUP | DRAG | DRAG_POPUP | MOVE | WHEEL | GESTURE | TIME | KEYBOARD)
	, editor(editor)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	model = std::make_shared<MapViewModel>();
	model->setLevel(0);
	model->setTileSize(Point(NATIVE_TILE, NATIVE_TILE));
	model->setViewDimensions(pos.dimensions());
	model->setViewCenter(pos.dimensions() / 2);
}

EditorMapView::~EditorMapView() = default;

void EditorMapView::reload()
{
	resetInteraction();

	if(!editor.hasMap())
	{
		context.reset();
		state.reset();
		cache.reset();
		return;
	}

	level = std::clamp(level, 0, editor.levels() - 1);
	model->setLevel(level);

	state = std::make_shared<MapRendererContextState>(*editor.getMap());
	context = std::make_shared<EditorMapRendererContext>(*state, *editor.getMap());
	context->setLockedObjects(editor.getLockedObjects(level));
	cache = std::make_shared<MapViewCache>(model, false);

	notifyViewChanged();
}

void EditorMapView::refreshLocked()
{
	if(context)
		context->setLockedObjects(editor.getLockedObjects(level));
}

void EditorMapView::invalidateObjectTiles(const CGObjectInstance * object)
{
	if(cache && context)
		cache->invalidate(context, object->id);
}

void EditorMapView::objectAdded(const CGObjectInstance * object)
{
	if(!state || !context)
		return;

	state->addObject(object);
	cache->invalidate(context, object->id);
}

void EditorMapView::objectRemoving(const CGObjectInstance * object)
{
	if(!state || !context)
		return;

	cache->invalidate(context, object->id);
	state->removeObject(object);
}

void EditorMapView::resetInteraction()
{
	leftDown = false;
	rightDown = false;
	pressedOnSelected = false;
	selectionMode = SelectionMode::NOTHING;
	shift = Point(0, 0);
	tileStart = int3(0, 0, level);
	tilePrev = int3(0, 0, level);
	temporaryTiles.clear();
}

void EditorMapView::setLevel(int newLevel)
{
	if(newLevel == level)
		return;

	level = newLevel;
	model->setLevel(level);
	resetInteraction();
	refreshLocked();
	notifyViewChanged();
}

int EditorMapView::getLevel() const
{
	return level;
}

void EditorMapView::setTool(Tool newTool)
{
	tool = newTool;
	resetInteraction();
}

EditorMapView::Tool EditorMapView::getTool() const
{
	return tool;
}

void EditorMapView::setGridVisible(bool on)
{
	gridVisible = on;
}

bool EditorMapView::isGridVisible() const
{
	return gridVisible;
}

void EditorMapView::setPassabilityVisible(bool on)
{
	passabilityVisible = on;
}

bool EditorMapView::isPassabilityVisible() const
{
	return passabilityVisible;
}

void EditorMapView::setPendingObject(const std::shared_ptr<CGObjectInstance> & object)
{
	pendingObject = object;
	shift = Point(0, 0);
}

std::shared_ptr<CGObjectInstance> EditorMapView::getPendingObject() const
{
	return pendingObject;
}

bool EditorMapView::isPicking() const
{
	return static_cast<bool>(pickCallback);
}

void EditorMapView::startPicking(const std::function<bool(const CGObjectInstance *)> & filter, const std::function<void(const CGObjectInstance *)> & callback)
{
	pickFilter = filter;
	pickCallback = callback;
	pickable.clear();

	if(!editor.hasMap())
		return;

	for(const auto & object : editor.getMap()->objects)
		if(object && filter(object.get()))
			pickable.insert(object.get());
}

void EditorMapView::zoom(int steps)
{
	if(steps == 0)
		return;

	const int oldSize = model->getSingleTileSize().x;
	int newSize = oldSize;
	for(int i = 0; i < std::abs(steps); ++i)
	{
		const int step = std::max(2, newSize / 5);
		newSize += steps > 0 ? step : -step;
	}
	newSize = std::clamp(newSize, MIN_TILE_SIZE, MAX_TILE_SIZE);

	if(newSize == oldSize)
		return;

	const Point center = model->getMapViewCenter();
	model->setTileSize(Point(newSize, newSize));
	model->setViewCenter(center * newSize / oldSize);
	notifyViewChanged();
}

void EditorMapView::resetZoom()
{
	const int oldSize = model->getSingleTileSize().x;
	const Point center = model->getMapViewCenter();

	model->setTileSize(Point(NATIVE_TILE, NATIVE_TILE));
	model->setViewCenter(center * NATIVE_TILE / oldSize);
	notifyViewChanged();
}

void EditorMapView::resetView()
{
	model->setTileSize(Point(NATIVE_TILE, NATIVE_TILE));
	model->setViewCenter(pos.dimensions() / 2);
	notifyViewChanged();
}

Point EditorMapView::getTileSize() const
{
	return model->getSingleTileSize();
}

void EditorMapView::scrollBy(const Point & pixels)
{
	Point center = model->getMapViewCenter() + pixels;

	if(editor.hasMap())
	{
		const Point tileSize = model->getSingleTileSize();
		center.x = std::clamp(center.x, 0, editor.getMap()->width * tileSize.x);
		center.y = std::clamp(center.y, 0, editor.getMap()->height * tileSize.y);
	}

	model->setViewCenter(center);
	notifyViewChanged();
}

void EditorMapView::centerOnTile(const Point & tile)
{
	const Point tileSize = model->getSingleTileSize();
	model->setViewCenter(tile * tileSize + tileSize / 2);
	notifyViewChanged();
}

Rect EditorMapView::getVisibleTiles() const
{
	const Point tileSize = model->getSingleTileSize();
	const Point topLeft = model->getMapViewCenter() - pos.dimensions() / 2;

	return Rect(topLeft / tileSize, Point(pos.w / tileSize.x + 1, pos.h / tileSize.y + 1));
}

void EditorMapView::notifyViewChanged()
{
	if(onViewChanged)
		onViewChanged();
}

void EditorMapView::exportLevel(const boost::filesystem::path & path)
{
	if(cache && context)
		cache->exportMapLevel(context, level, path);
}

int3 EditorMapView::tileAt(const Point & cursor) const
{
	return model->getTileAtPoint(cursor - pos.topLeft());
}

std::vector<int3> EditorMapView::brushTiles(const int3 & tile) const
{
	std::vector<int3> result;

	switch(tool)
	{
		case Tool::BRUSH2:
			for(int dy = 0; dy < 2; ++dy)
				for(int dx = 0; dx < 2; ++dx)
					result.emplace_back(tile.x + dx, tile.y + dy, tile.z);
			break;

		case Tool::BRUSH4:
			for(int dy = -1; dy < 3; ++dy)
				for(int dx = -1; dx < 3; ++dx)
					result.emplace_back(tile.x + dx, tile.y + dy, tile.z);
			break;

		default:
			result.push_back(tile);
			break;
	}
	return result;
}

void EditorMapView::fillFrom(const int3 & start, bool select)
{
	const auto & map = *editor.getMap();
	const auto & selection = editor.getTerrainSelection(level);

	std::vector<int3> queue;
	std::set<int3> tilesToFill;
	queue.push_back(start);

	const std::array<int3, 4> dirs{int3{1, 0, 0}, int3{-1, 0, 0}, int3{0, 1, 0}, int3{0, -1, 0}};

	while(!queue.empty())
	{
		auto tile = queue.back();
		queue.pop_back();
		tilesToFill.insert(tile);

		for(auto & d : dirs)
		{
			auto next = tile + d;
			if(tilesToFill.count(next) || !map.isInTheMap(next))
				continue;

			if(select)
			{
				const auto & current = map.getTile(tile);
				const auto & neighbour = map.getTile(next);

				if(current.roadType && current.roadType != neighbour.roadType)
					continue;
				if(current.riverType && current.riverType != neighbour.riverType)
					continue;
				if(current.terrainType != neighbour.terrainType)
					continue;
				if(selection.count(next))
					continue;
			}
			else if(!selection.count(next))
				continue;

			queue.push_back(next);
		}
	}

	std::vector<int3> result(tilesToFill.begin(), tilesToFill.end());
	if(select)
		editor.selectTiles(level, result);
	else
		editor.deselectTiles(level, result);
}

void EditorMapView::lassoFinish()
{
	std::vector<int3> closing;

	//connect with initial tile
	for(auto i = tilePrev; i != tileStart;)
	{
		int length = std::numeric_limits<int>::max();
		int3 dir;
		for(auto & d : int3::getDirs())
		{
			if(tileStart.dist2dSQ(i + d) < length)
			{
				dir = d;
				length = tileStart.dist2dSQ(i + d);
			}
		}
		i += dir;
		closing.push_back(i);
	}
	editor.selectTiles(level, closing);

	//key: y position of tile, value: x positions of the left and the right tile
	std::map<int, std::pair<int, int>> rangesX;
	std::map<int, std::pair<int, int>> rangesY;
	for(auto & t : editor.getTerrainSelection(level))
	{
		auto it = rangesX.find(t.y);
		if(it == rangesX.end())
			rangesX[t.y] = std::make_pair(t.x, t.x);
		else
		{
			it->second.first = std::min(it->second.first, t.x);
			it->second.second = std::max(it->second.second, t.x);
		}

		it = rangesY.find(t.x);
		if(it == rangesY.end())
			rangesY[t.x] = std::make_pair(t.y, t.y);
		else
		{
			it->second.first = std::min(it->second.first, t.y);
			it->second.second = std::max(it->second.second, t.y);
		}
	}

	std::set<int3> byX;
	std::set<int3> byY;
	std::vector<int3> inside;
	for(auto & range : rangesX)
		for(int i = range.second.first; i < range.second.second; ++i)
			byX.insert(int3(i, range.first, level));
	for(auto & range : rangesY)
		for(int i = range.second.first; i < range.second.second; ++i)
			byY.insert(int3(range.first, i, level));

	std::set_intersection(byX.begin(), byX.end(), byY.begin(), byY.end(), std::back_inserter(inside));
	editor.selectTiles(level, inside);
}

void EditorMapView::handlePress(const Point & cursor, bool rightButton)
{
	if(!editor.hasMap())
		return;

	const int3 tile = tileAt(cursor);
	tileStart = tilePrev = tile;
	hoveredTile = tile;

	if(isPicking())
	{
		auto callback = pickCallback;
		const CGObjectInstance * picked = nullptr;
		if(!rightButton)
		{
			auto * object = editor.findObjectAt(tile);
			if(object && pickable.count(object))
				picked = object;
		}

		pickCallback = nullptr;
		pickFilter = nullptr;
		pickable.clear();
		callback(picked);
		return;
	}

	if(pendingObject)
	{
		if(rightButton)
		{
			pendingObject.reset();
			if(onPlacementCancelled)
				onPlacementCancelled();
		}
		else if(editor.getMap()->isInTheMap(tile) && onPlacementRequested)
			onPlacementRequested(tile);
		return;
	}

	pressedOnSelected = editor.getTerrainSelection(level).count(tile) != 0;

	switch(tool)
	{
		case Tool::PAN:
			break;

		case Tool::BRUSH:
		case Tool::BRUSH2:
		case Tool::BRUSH4:
		case Tool::LINE:
			editor.clearObjectSelection(level);
			if(rightButton)
				editor.deselectTiles(level, brushTiles(tile));
			else
				editor.selectTiles(level, brushTiles(tile));
			break;

		case Tool::AREA:
		case Tool::LASSO:
			if(rightButton)
				break;

			editor.clearTerrainSelection(level);
			editor.clearObjectSelection(level);
			break;

		case Tool::FILL:
			if(editor.getMap()->isInTheMap(tile))
				fillFrom(tile, !rightButton);
			editor.clearObjectSelection(level);
			break;

		case Tool::SELECT:
		{
			editor.clearTerrainSelection(level);
			if(rightButton)
				break;

			const bool additive = ENGINE->isKeyboardCtrlDown();

			//the object can be over another one, so the second one is needed to choose which to select
			auto * first = editor.findObjectAt(tile);
			auto * second = editor.findObjectAt(tile, first);

			if(first)
			{
				if(editor.isSelected(level, first))
				{
					if(additive)
					{
						editor.deselectObject(level, first);
						selectionMode = SelectionMode::SELECTION;
					}
					else
						selectionMode = SelectionMode::MOVEMENT;
				}
				else
				{
					if(!second || !editor.isSelected(level, second))
					{
						if(!additive)
							editor.clearObjectSelection(level);
						editor.selectObject(level, first);
					}
					selectionMode = SelectionMode::MOVEMENT;
				}
			}
			else
			{
				editor.clearObjectSelection(level);
				selectionMode = SelectionMode::SELECTION;
				rubberStart = rubberEnd = cursor - pos.topLeft();
			}
			shift = Point(0, 0);
			break;
		}
	}
}

void EditorMapView::handleMove(const Point & cursor)
{
	const int3 tile = tileAt(cursor);
	hoveredTile = tile;

	if(onTileHovered && editor.hasMap() && editor.getMap()->isInTheMap(tile))
		onTileHovered(tile);

	if(!editor.hasMap() || pendingObject || isPicking())
		return;

	if(selectionMode == SelectionMode::SELECTION)
		rubberEnd = cursor - pos.topLeft();

	if(tile == tilePrev)
		return;

	switch(tool)
	{
		case Tool::PAN:
		case Tool::FILL:
			break;

		case Tool::BRUSH:
		case Tool::BRUSH2:
		case Tool::BRUSH4:
			if(rightDown)
				editor.deselectTiles(level, brushTiles(tile));
			else if(leftDown)
				editor.selectTiles(level, brushTiles(tile));
			break;

		case Tool::AREA:
		{
			if(rightDown || !leftDown)
				break;

			editor.clearTerrainSelection(level);
			std::vector<int3> selected;
			for(int y = std::min(tile.y, tileStart.y); y <= std::max(tile.y, tileStart.y); ++y)
				for(int x = std::min(tile.x, tileStart.x); x <= std::max(tile.x, tileStart.x); ++x)
					selected.emplace_back(x, y, level);

			editor.selectTiles(level, selected);
			break;
		}

		case Tool::LINE:
		{
			if(!leftDown && !rightDown)
				break;

			const auto diff = tile - tileStart;
			if(diff == int3{})
				break;

			const int edge = std::max(std::abs(diff.x), std::abs(diff.y));
			int distMin = std::numeric_limits<int>::max();
			int3 dir;

			for(auto & d : int3::getDirs())
			{
				if(tile.dist2d(d * edge + tileStart) < distMin)
				{
					distMin = tile.dist2d(d * edge + tileStart);
					dir = d;
				}
			}

			if(leftDown)
			{
				std::vector<int3> erased(temporaryTiles.begin(), temporaryTiles.end());
				editor.deselectTiles(level, erased);
				temporaryTiles.clear();

				std::vector<int3> selected;
				for(auto ts = tileStart; ts.dist2d(tileStart) < edge; ts += dir)
				{
					if(!editor.getMap()->isInTheMap(ts))
						break;
					if(!editor.getTerrainSelection(level).count(ts))
						temporaryTiles.insert(ts);
					selected.push_back(ts);
				}
				editor.selectTiles(level, selected);
			}
			else
			{
				std::vector<int3> erased;
				for(auto ts = tileStart; ts.dist2d(tileStart) < edge; ts += dir)
				{
					if(!editor.getMap()->isInTheMap(ts))
						break;
					erased.push_back(ts);
				}
				editor.deselectTiles(level, erased);
			}
			break;
		}

		case Tool::LASSO:
		{
			if(!leftDown)
				break;

			std::vector<int3> tiles;
			for(auto i = tilePrev; i != tile;)
			{
				int length = std::numeric_limits<int>::max();
				int3 dir;
				for(auto & d : int3::getDirs())
				{
					if(tile.dist2dSQ(i + d) < length)
					{
						dir = d;
						length = tile.dist2dSQ(i + d);
					}
				}
				i += dir;
				tiles.push_back(i);
			}
			editor.selectTiles(level, tiles);
			break;
		}

		case Tool::SELECT:
		{
			if(rightDown || !leftDown)
				break;

			shift = Point(tile.x - tileStart.x, tile.y - tileStart.y);

			if(selectionMode == SelectionMode::SELECTION && (shift.x != 0 || shift.y != 0))
			{
				editor.clearObjectSelection(level);
				editor.selectObjects(level, editor.findObjectsIn(level, tileStart, tile));
			}
			break;
		}
	}

	tilePrev = tile;
}

void EditorMapView::handleRelease(const Point & cursor, bool rightButton)
{
	if(!editor.hasMap() || pendingObject || isPicking())
		return;

	switch(tool)
	{
		case Tool::LASSO:
			if(!rightButton)
				lassoFinish();
			break;

		case Tool::LINE:
			temporaryTiles.clear();
			break;

		case Tool::SELECT:
		{
			if(rightButton)
				break;

			bool openProperties = false;
			if(selectionMode == SelectionMode::MOVEMENT)
			{
				openProperties = shift.x == 0 && shift.y == 0;
				editor.commitObjectShift(level, shift);
			}
			else
				openProperties = true;

			selectionMode = SelectionMode::NOTHING;
			shift = Point(0, 0);

			const auto & selection = editor.getObjectSelection(level);
			if(selection.size() == 1 && onObjectOpened)
				onObjectOpened(*selection.begin(), openProperties);
			break;
		}

		default:
			break;
	}
}

void EditorMapView::clickPressed(const Point & cursorPosition)
{
	leftDown = true;
	handlePress(cursorPosition, false);
}

void EditorMapView::clickReleased(const Point & cursorPosition)
{
	if(!leftDown)
		return;

	leftDown = false;
	handleRelease(cursorPosition, false);
}

void EditorMapView::clickCancel(const Point & cursorPosition)
{
	leftDown = false;
	selectionMode = SelectionMode::NOTHING;
	shift = Point(0, 0);
}

void EditorMapView::showPopupWindow(const Point & cursorPosition)
{
	rightDown = true;
	handlePress(cursorPosition, true);
}

void EditorMapView::closePopupWindow(bool alreadyClosed)
{
	if(!rightDown)
		return;

	rightDown = false;
	handleRelease(ENGINE->getCursorPosition(), true);
}

void EditorMapView::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	handleMove(cursorPosition);
}

void EditorMapView::mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	if(tool == Tool::PAN && !pendingObject)
	{
		scrollBy(lastUpdateDistance);
		return;
	}

	handleMove(cursorPosition);
}

void EditorMapView::mouseDraggedPopup(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	handleMove(cursorPosition);
}

void EditorMapView::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	scrollBy(lastUpdateDistance);
}

void EditorMapView::gesturePinch(const Point & centerPosition, double lastUpdateFactor)
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

void EditorMapView::wheelScrolled(int distance)
{
	if(ENGINE->isKeyboardCtrlDown())
		zoom(distance);
	else if(ENGINE->isKeyboardShiftDown())
		scrollBy(Point(-distance * model->getSingleTileSize().x * 2, 0));
	else
		scrollBy(Point(0, -distance * model->getSingleTileSize().y * 2));
}

void EditorMapView::keyPressed(EShortcut key)
{
	switch(key)
	{
		case EShortcut::EDITOR_SCROLL_LEFT:
			scrollDirection.x = -1;
			break;
		case EShortcut::EDITOR_SCROLL_RIGHT:
			scrollDirection.x = 1;
			break;
		case EShortcut::EDITOR_SCROLL_UP:
			scrollDirection.y = -1;
			break;
		case EShortcut::EDITOR_SCROLL_DOWN:
			scrollDirection.y = 1;
			break;
		case EShortcut::GLOBAL_CANCEL:
			if(pendingObject)
			{
				pendingObject.reset();
				if(onPlacementCancelled)
					onPlacementCancelled();
			}
			break;
		default:
			break;
	}
}

void EditorMapView::keyReleased(EShortcut key)
{
	switch(key)
	{
		case EShortcut::EDITOR_SCROLL_LEFT:
			if(scrollDirection.x < 0)
				scrollDirection.x = 0;
			break;
		case EShortcut::EDITOR_SCROLL_RIGHT:
			if(scrollDirection.x > 0)
				scrollDirection.x = 0;
			break;
		case EShortcut::EDITOR_SCROLL_UP:
			if(scrollDirection.y < 0)
				scrollDirection.y = 0;
			break;
		case EShortcut::EDITOR_SCROLL_DOWN:
			if(scrollDirection.y > 0)
				scrollDirection.y = 0;
			break;
		default:
			break;
	}
}

void EditorMapView::tick(uint32_t msPassed)
{
	if(scrollDirection.x == 0 && scrollDirection.y == 0)
		return;

	const int distance = std::max(1, static_cast<int>(msPassed * SCROLL_SPEED));
	scrollBy(scrollDirection * distance);
}

std::shared_ptr<IImage> EditorMapView::getGhostImage(const CGObjectInstance * object)
{
	if(!object->appearance || object->appearance->animationFile.empty())
		return nullptr;

	const auto & path = object->appearance->animationFile;
	auto it = ghostAnimations.find(path);
	if(it == ghostAnimations.end())
		it = ghostAnimations.emplace(path, ENGINE->renderHandler().loadAnimation(path, EImageBlitMode::WITH_SHADOW)).first;

	const auto & animation = it->second;
	size_t group = object->ID == Obj::HERO ? 2 : 0;
	if(animation->size(group) == 0)
		group = 0;
	if(animation->size(group) == 0)
		return nullptr;

	return animation->getImage(0, group);
}

Rect EditorMapView::objectArea(const CGObjectInstance * object, const Point & tilesShift) const
{
	int minX = object->anchorPos().x;
	int minY = object->anchorPos().y;
	int maxX = minX;
	int maxY = minY;

	for(const auto & tile : object->getBlockedPos())
	{
		minX = std::min(minX, tile.x);
		minY = std::min(minY, tile.y);
		maxX = std::max(maxX, tile.x);
		maxY = std::max(maxY, tile.y);
	}

	const Rect first = model->getTargetTileArea(int3(minX + tilesShift.x, minY + tilesShift.y, level));
	const Point tileSize = model->getSingleTileSize();
	return Rect(first.topLeft(), Point(maxX - minX + 1, maxY - minY + 1) * tileSize);
}

void EditorMapView::drawGhost(Canvas & to, const CGObjectInstance * object, const int3 & anchorTile)
{
	auto image = getGhostImage(object);
	const Point tileSize = model->getSingleTileSize();

	if(!image)
	{
		const Rect area = model->getTargetTileArea(anchorTile);
		to.drawColorBlended(area, PICKABLE_COLOR);
		return;
	}

	const Point dimensions = image->dimensions();
	const Point scaled = dimensions * tileSize / NATIVE_TILE;
	const Rect anchorArea = model->getTargetTileArea(anchorTile);
	const Point topLeft = anchorArea.bottomRight() - scaled;

	image->setAlpha(GHOST_ALPHA);

	if(tileSize.x == NATIVE_TILE && tileSize.y == NATIVE_TILE)
	{
		to.draw(image, topLeft);
		return;
	}

	Canvas scratch(dimensions, CanvasScalingPolicy::IGNORE);
	scratch.applyTransparency(true);
	scratch.draw(image, Point(0, 0));
	to.drawScaled(scratch, topLeft, scaled);
}

void EditorMapView::drawOverlays(Canvas & to)
{
	const auto & map = *editor.getMap();
	const Rect tiles = model->getTilesTotalRect();
	const auto & tileSelection = editor.getTerrainSelection(level);

	for(int y = tiles.top(); y < tiles.bottom(); ++y)
	{
		for(int x = tiles.left(); x < tiles.right(); ++x)
		{
			const int3 tile(x, y, level);
			if(!map.isInTheMap(tile))
				continue;

			const Rect area = model->getTargetTileArea(tile);

			if(passabilityVisible)
			{
				const auto & terrainTile = map.getTile(tile);
				if(terrainTile.blocked() || terrainTile.visitable())
					to.drawColorBlended(Rect(area.x, area.y, area.w - 1, area.h - 1), terrainTile.visitable() ? VISITABLE_COLOR : BLOCKED_COLOR);
			}

			if(tileSelection.count(tile))
				to.drawColorBlended(Rect(area.x, area.y, area.w - 1, area.h - 1), TILE_SELECTION_COLOR);

			if(gridVisible)
			{
				to.drawColorBlended(Rect(area.right() - 1, area.y, 1, area.h), GRID_COLOR);
				to.drawColorBlended(Rect(area.x, area.bottom() - 1, area.w, 1), GRID_COLOR);
			}
		}
	}

	for(const auto * object : pickable)
	{
		if(object->pos.z != level)
			continue;

		for(const auto & tile : object->getBlockedPos())
			to.drawColorBlended(model->getTargetTileArea(tile), PICKABLE_COLOR);
	}

	const bool moving = selectionMode == SelectionMode::MOVEMENT && (shift.x != 0 || shift.y != 0);
	size_t ghosts = 0;

	for(auto * object : editor.getObjectSelection(level))
	{
		to.drawBorder(objectArea(object, Point(0, 0)).resize(-1), Colors::WHITE_TRUE);

		if(moving)
		{
			if(ghosts++ < MAX_GHOST_OBJECTS)
				drawGhost(to, object, object->anchorPos() + int3(shift.x, shift.y, 0));
			else
				to.drawBorder(objectArea(object, shift), Colors::WHITE_TRUE);
		}
	}

	if(pendingObject && map.isInTheMap(hoveredTile))
	{
		drawGhost(to, pendingObject.get(), int3(hoveredTile.x, hoveredTile.y, level));
		to.drawBorder(model->getTargetTileArea(int3(hoveredTile.x, hoveredTile.y, level)), Colors::WHITE_TRUE);
	}

	if(selectionMode == SelectionMode::SELECTION && (rubberStart.x != rubberEnd.x || rubberStart.y != rubberEnd.y))
	{
		const Rect band(Point(std::min(rubberStart.x, rubberEnd.x), std::min(rubberStart.y, rubberEnd.y)),
			Point(std::abs(rubberStart.x - rubberEnd.x), std::abs(rubberStart.y - rubberEnd.y)));
		to.drawColorBlended(band, RUBBER_BAND_FILL);
		to.drawBorder(band, Colors::WHITE_TRUE);
	}
}

void EditorMapView::render(Canvas & to)
{
	CanvasClipRectGuard guard(to, pos);

	if(!editor.hasMap() || !cache || !context)
	{
		to.drawColor(pos, Colors::BLACK);
		return;
	}

	Canvas view(to, pos);
	cache->update(context);
	cache->render(context, view, true);
	drawOverlays(view);
}

void EditorMapView::show(Canvas & to)
{
	render(to);
}

void EditorMapView::showAll(Canvas & to)
{
	render(to);
}
