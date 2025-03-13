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
	void selectionMade(bool anythingSelected);

private:
	std::set<int3> area;
	std::set<int3> areaAdd;
	std::set<int3> areaErase;

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
	
	void draw(bool onlyDirty = true);
	
	void setDirty(int x, int y);
	void setDirty(const CGObjectInstance * object);

	void setLockObject(const CGObjectInstance * object, bool lock);
	void unlockAll();
	
private:
	std::set<const CGObjectInstance *> objDirty;
	std::set<const CGObjectInstance *> lockedObjects;
	std::set<int3> dirty;
};


class ObjectPickerLayer: public AbstractLayer
{
	Q_OBJECT
public:
	ObjectPickerLayer(MapSceneBase * s);
	
	void update() override;
	bool isVisible() const;
	
	template<class T>
	void highlight()
	{
		highlight([](const CGObjectInstance * o){ return dynamic_cast<T*>(o); });
	}
	
	void highlight(std::function<bool(const CGObjectInstance *)> predicate);
	
	void clear();
	
	void select(const CGObjectInstance *);
	void discard();
	
signals:
	void selectionMade(const CGObjectInstance *);
	
private:
	bool isActive = false;
	std::set<const CGObjectInstance *> possibleObjects;
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
	
	CGObjectInstance * selectObjectAt(int x, int y, const CGObjectInstance * ignore = nullptr) const;
	void selectObjects(int x1, int y1, int x2, int y2);
	void selectObject(CGObjectInstance *, bool inform = true);
	void deselectObject(CGObjectInstance *);
	bool isSelected(const CGObjectInstance *) const;
	std::set<CGObjectInstance*> getSelection() const;
	void clear();

	void setLockObject(const CGObjectInstance * object, bool lock);
	void unlockAll();
		
	QPoint shift;
	std::shared_ptr<CGObjectInstance> newObject;
	//FIXME: magic number
	SelectionMode selectionMode = SelectionMode::NOTHING;

signals:
	void selectionMade(bool anythingSelected);
	
private:
	std::set<CGObjectInstance *> selectedObjects;
	std::set<const CGObjectInstance *> lockedObjects;

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
	int x = 0;
	int y = 0;
	int w = 1;
	int h = 1;
	
};
