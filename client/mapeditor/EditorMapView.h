/*
 * EditorMapView.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorMap.h"

#include "../gui/CIntObject.h"
#include "../../lib/filesystem/ResourcePath.h"

class CAnimation;
class IImage;
class MapViewModel;
class MapViewCache;
class EditorMapRendererContext;
struct MapRendererContextState;

/// Interactive view of one level of the map opened in the editor
class EditorMapView : public CIntObject
{
public:
	enum class Tool
	{
		SELECT,
		PAN,
		BRUSH,
		BRUSH2,
		BRUSH4,
		AREA,
		LASSO,
		LINE,
		FILL
	};

private:
	enum class SelectionMode
	{
		NOTHING,
		SELECTION,
		MOVEMENT
	};

	EditorMap & editor;
	int level = 0;
	Tool tool = Tool::SELECT;

	std::shared_ptr<MapViewModel> model;
	std::shared_ptr<MapViewCache> cache;
	std::shared_ptr<MapRendererContextState> state;
	std::shared_ptr<EditorMapRendererContext> context;
	std::map<AnimationPath, std::shared_ptr<CAnimation>> ghostAnimations;

	bool gridVisible = false;
	bool passabilityVisible = false;

	std::shared_ptr<CGObjectInstance> pendingObject;
	std::function<bool(const CGObjectInstance *)> pickFilter;
	std::function<void(const CGObjectInstance *)> pickCallback;
	std::set<const CGObjectInstance *> pickable;

	bool leftDown = false;
	bool rightDown = false;
	bool pressedOnSelected = false;
	SelectionMode selectionMode = SelectionMode::NOTHING;
	int3 tileStart;
	int3 tilePrev;
	int3 hoveredTile;
	Point shift;
	Point rubberStart;
	Point rubberEnd;
	Point scrollDirection;
	double pinchFactor = 1.0;
	std::set<int3> temporaryTiles;

	int3 tileAt(const Point & cursor) const;
	std::vector<int3> brushTiles(const int3 & tile) const;
	std::shared_ptr<IImage> getGhostImage(const CGObjectInstance * object);
	bool isPicking() const;

	void resetInteraction();
	void handlePress(const Point & cursor, bool rightButton);
	void handleMove(const Point & cursor);
	void handleRelease(const Point & cursor, bool rightButton);
	void fillFrom(const int3 & start, bool select);
	void lassoFinish();
	void notifyViewChanged();

	void render(Canvas & to);
	void drawOverlays(Canvas & to);
	void drawGhost(Canvas & to, const CGObjectInstance * object, const int3 & anchorTile);
	Rect objectArea(const CGObjectInstance * object, const Point & tilesShift) const;

public:
	std::function<void(const int3 &)> onTileHovered;
	/// Called when a single object is selected, the flag tells to show the properties of the object
	std::function<void(CGObjectInstance *, bool)> onObjectOpened;
	/// Called when the pending object is clicked into the map
	std::function<void(const int3 &)> onPlacementRequested;
	std::function<void()> onPlacementCancelled;
	std::function<void()> onViewChanged;

	EditorMapView(const Rect & area, EditorMap & editor);
	~EditorMapView() override;

	/// Rebuilds everything the view shows, e.g. after a map was loaded or objects were changed in bulk
	void reload();
	void invalidateObjectTiles(const CGObjectInstance * object);
	void objectAdded(const CGObjectInstance * object);
	void objectRemoving(const CGObjectInstance * object);
	void refreshLocked();

	void setLevel(int newLevel);
	int getLevel() const;
	void setTool(Tool newTool);
	Tool getTool() const;
	void setGridVisible(bool on);
	bool isGridVisible() const;
	void setPassabilityVisible(bool on);
	bool isPassabilityVisible() const;

	/// Object that follows the cursor until it is placed or the placement is cancelled
	void setPendingObject(const std::shared_ptr<CGObjectInstance> & object);
	std::shared_ptr<CGObjectInstance> getPendingObject() const;
	/// Highlights the objects accepted by the filter, the next click on one of them calls the callback.
	/// A click elsewhere calls it with nullptr
	void startPicking(const std::function<bool(const CGObjectInstance *)> & filter, const std::function<void(const CGObjectInstance *)> & callback);

	void zoom(int steps);
	void resetZoom();
	/// Resets the zoom and shows the top left corner of the map
	void resetView();
	void scrollBy(const Point & pixels);
	void centerOnTile(const Point & tile);
	/// Visible part of the map in tiles
	Rect getVisibleTiles() const;
	Point getTileSize() const;

	/// Saves the whole level to a PNG file
	void exportLevel(const boost::filesystem::path & path);

	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickCancel(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void closePopupWindow(bool alreadyClosed) override;
	void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void mouseDraggedPopup(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesturePinch(const Point & centerPosition, double lastUpdateFactor) override;
	void wheelScrolled(int distance) override;
	void keyPressed(EShortcut key) override;
	void keyReleased(EShortcut key) override;
	void tick(uint32_t msPassed) override;
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
};
