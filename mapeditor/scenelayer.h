/*
 * scenelayer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/int3.h"

class MapSceneBase;
class MapScene;
class MapController;
class MapHandler;

VCMI_LIB_NAMESPACE_BEGIN
class CMap;
class CGObjectInstance;
VCMI_LIB_NAMESPACE_END


class AbstractLayer : public QObject
{
	Q_OBJECT
public:
	AbstractLayer(MapSceneBase * s);
	void initialize(MapController & controller);
	void show(bool show);
	virtual void update() = 0;
	virtual void redraw() = 0;

protected:
	int mapWidthPx() const;
	int mapHeightPx() const;
	int toInt(double value) const;

	MapSceneBase * scene;
	CMap * map = nullptr;
	MapHandler * handler = nullptr;
	bool isShown = false;
	const int tileSize = 32;
};

class AbstractFixedLayer : public AbstractLayer
{
	Q_OBJECT
public:
	AbstractFixedLayer(MapSceneBase * s);
	void redraw();

protected:
	std::unique_ptr<QPixmap> pixmap;
	QPixmap emptyPixmap;

private:
	std::unique_ptr<QGraphicsPixmapItem> item;
};

class AbstractViewportLayer : public AbstractLayer
{
public:
	AbstractViewportLayer(MapSceneBase * s);
	void createLayer();
	void setViewport(const QRectF & _viewPort);

	void update();
	void redraw();
protected:
	virtual QGraphicsItem * draw(const QRectF & area) = 0;
	void redraw(const std::vector<int3> & tiles);
	void redrawWithSurroundingTiles(const std::vector<int3> & tiles);
	void redraw(const std::set<CGObjectInstance *> & objects);
	void redraw(const std::vector<QRectF> & areas);
	QRectF getObjectArea(const CGObjectInstance * object) const;
private:
	void addSector(QGraphicsItem * item);
	void removeSector(QGraphicsItem * item);
	void redrawSectors(std::set<QGraphicsItem *> & items);
	const QList<QGraphicsItem *> getAllSectors() const;

	std::set<QGraphicsItem *> getContainingSectors(const std::vector<int3> & tiles) const;
	std::set<QGraphicsItem *> getIntersectingSectors(const std::vector<QRectF> & areas) const;
	std::unique_ptr<QGraphicsItemGroup> items;
	const int sectorSizeInTiles = 10;
	const int sectorSize = sectorSizeInTiles * tileSize;
};

class EmptyLayer: public AbstractFixedLayer
{
	Q_OBJECT
public:
	EmptyLayer(MapSceneBase * s);

	void update() override;
};

class GridLayer: public AbstractViewportLayer
{
	Q_OBJECT
public:
	GridLayer(MapSceneBase * s);

protected:
	QGraphicsItem * draw(const QRectF & section) override;
};

class PassabilityLayer: public AbstractViewportLayer
{
	Q_OBJECT
public:
	PassabilityLayer(MapSceneBase * s);

protected:
	QGraphicsItem * draw(const QRectF & section) override;
};

class SelectionTerrainLayer: public AbstractViewportLayer
{
	Q_OBJECT
public:
	SelectionTerrainLayer(MapSceneBase* s);

	void select(const std::vector<int3> & tiles);
	void erase(const std::vector<int3> & tiles);
	void clear();

	const std::set<int3> & selection() const;

protected:
	QGraphicsItem * draw(const QRectF & section) override;

signals:
	void selectionMade(bool anythingSelected);

private:
	std::set<int3> area;
	std::set<int3> areaAdd;
	std::set<int3> areaErase;

	void onSelection();
};


class TerrainLayer: public AbstractViewportLayer
{
	Q_OBJECT
public:
	TerrainLayer(MapSceneBase * s);
	void redrawTerrain(const std::vector<int3> & tiles);

protected:
	QGraphicsItem * draw(const QRectF & section) override;
};


class ObjectsLayer: public AbstractViewportLayer
{
	Q_OBJECT
public:
	ObjectsLayer(MapSceneBase * s);

	void redrawObjects(const std::set<CGObjectInstance *> & objects);
	void setLockObject(const CGObjectInstance * object, bool lock);
	void unlockAll();
protected:
	QGraphicsItem * draw(const QRectF & section) override;
private:
	std::set<const CGObjectInstance *> lockedObjects;
};


class ObjectPickerLayer: public AbstractViewportLayer
{
	Q_OBJECT
public:
	ObjectPickerLayer(MapSceneBase * s);
	bool isVisible() const;
	
	template<class T>
	void highlight()
	{
		highlight([](const CGObjectInstance * o){ return dynamic_cast<T*>(o); });
	}

	void highlight(const std::function<bool(const CGObjectInstance *)> & predicate);

	void clear();
	
	void select(const CGObjectInstance *);
	void discard();
protected:
	QGraphicsItem * draw(const QRectF & section) override;

signals:
	void selectionMade(const CGObjectInstance *);
	
private:
	bool isActive = false;
	std::set<const CGObjectInstance *> possibleObjects;
};


class SelectionObjectsLayer: public AbstractViewportLayer
{
	Q_OBJECT
public:
	enum SelectionMode
	{
		NOTHING, SELECTION, MOVEMENT
	};
	
	SelectionObjectsLayer(MapSceneBase* s);

	CGObjectInstance * selectObjectAt(int x, int y, const CGObjectInstance * ignore = nullptr) const;
	void selectObjects(int x1, int y1, int x2, int y2);
	void selectObject(CGObjectInstance *);
	void deselectObject(CGObjectInstance *);
	bool isSelected(const CGObjectInstance *) const;
	std::set<CGObjectInstance*> getSelection() const;
	void clear();

	void setShift(int x, int y);
	void setLockObject(CGObjectInstance * object, bool lock);
	void unlockAll();
	std::shared_ptr<CGObjectInstance> newObject;
	QPoint shift;
	SelectionMode selectionMode = SelectionMode::NOTHING;

protected:
	QGraphicsItem * draw(const QRectF & section) override;

signals:
	void selectionMade(bool anythingSelected);
	
private:
	void selectObjects(const std::set<CGObjectInstance *> & objs);
	std::set<CGObjectInstance *> selectedObjects;
	std::set<const CGObjectInstance *> lockedObjects;

	void onSelection();
};

class MinimapLayer: public AbstractFixedLayer
{
public:
	MinimapLayer(MapSceneBase * s);
	
	void update() override;
};

class MinimapViewLayer: public AbstractFixedLayer
{
public:
	MinimapViewLayer(MapSceneBase * s);
	
	void setViewport(int x, int y, int w, int h);
	
	void draw();
	void update() override;
	
	int viewportX() const {return x;}
	int viewportY() const {return y;}
	int viewportWidth() const {return w;}
	int viewportHeight() const {return h;}
	
private:
	int x = 0;
	int y = 0;
	int w = 1;
	int h = 1;
	
};
