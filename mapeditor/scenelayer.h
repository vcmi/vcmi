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
	
	virtual void update() = 0;
	
	void show(bool show);
	void redraw();
	void initialize(MapController & controller);
	
protected:
	MapSceneBase * scene;
	CMap * map = nullptr;
	MapHandler * handler = nullptr;
	bool isShown = false;
	
	std::unique_ptr<QPixmap> pixmap;
	QPixmap emptyPixmap;
	
private:
	std::unique_ptr<QGraphicsPixmapItem> item;
};


class GridLayer: public AbstractLayer
{
	Q_OBJECT
public:
	GridLayer(MapSceneBase * s);
	
	void update() override;
};

class PassabilityLayer: public AbstractLayer
{
	Q_OBJECT
public:
	PassabilityLayer(MapSceneBase * s);
	
	void update() override;
};

class SelectionTerrainLayer: public AbstractLayer
{
	Q_OBJECT
public:
	SelectionTerrainLayer(MapSceneBase* s);
	
	void update() override;
	
	void draw();
	void select(const int3 & tile);
	void erase(const int3 & tile);
	void clear();
	
	const std::set<int3> & selection() const;

signals:
	void selectionMade(bool anythingSlected);

private:
	std::set<int3> area, areaAdd, areaErase;

	void onSelection();
};


class TerrainLayer: public AbstractLayer
{
	Q_OBJECT
public:
	TerrainLayer(MapSceneBase * s);
	
	void update() override;
	
	void draw(bool onlyDirty = true);
	void setDirty(const int3 & tile);
	
private:
	std::set<int3> dirty;
};


class ObjectsLayer: public AbstractLayer
{
	Q_OBJECT
public:
	ObjectsLayer(MapSceneBase * s);
	
	void update() override;
	
	void draw(bool onlyDirty = true); //TODO: implement dirty
	
	void setDirty(int x, int y);
	void setDirty(const CGObjectInstance * object);
	
private:
	std::set<const CGObjectInstance *> objDirty;
	std::set<int3> dirty;
};


class SelectionObjectsLayer: public AbstractLayer
{
	Q_OBJECT
public:
	enum SelectionMode
	{
		NOTHING, SELECTION, MOVEMENT
	};
	
	SelectionObjectsLayer(MapSceneBase* s);
	
	void update() override;
	
	void draw();
	
	CGObjectInstance * selectObjectAt(int x, int y) const;
	void selectObjects(int x1, int y1, int x2, int y2);
	void selectObject(CGObjectInstance *, bool inform = true);
	void deselectObject(CGObjectInstance *);
	bool isSelected(const CGObjectInstance *) const;
	std::set<CGObjectInstance*> getSelection() const;
	void moveSelection(int x, int y);
	void clear();
		
	QPoint shift;
	CGObjectInstance * newObject;
	//FIXME: magic number
	SelectionMode selectionMode = SelectionMode::NOTHING;

signals:
	void selectionMade(bool anythingSlected);
	
private:
	std::set<CGObjectInstance *> selectedObjects;

	void onSelection();
};

class MinimapLayer: public AbstractLayer
{
public:
	MinimapLayer(MapSceneBase * s);
	
	void update() override;
};

class MinimapViewLayer: public AbstractLayer
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
	int x = 0, y = 0, w = 1, h = 1;
	
};
