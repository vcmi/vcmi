#ifndef SCENELAYER_H
#define SCENELAYER_H

#include "../lib/int3.h"

class MapScene;
class CGObjectInstance;
class MapController;
class CMap;
class MapHandler;

class AbstractLayer
{
public:
	AbstractLayer(MapScene * s);
	
	virtual void update() = 0;
	
	void show(bool show);
	void redraw();
	void initialize(MapController & controller);
	
protected:
	MapScene * scene;
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
public:
	GridLayer(MapScene * s);
	
	void update() override;
};

class PassabilityLayer: public AbstractLayer
{
public:
	PassabilityLayer(MapScene * s);
	
	void update() override;
};


class SelectionTerrainLayer: public AbstractLayer
{
public:
	SelectionTerrainLayer(MapScene * s);
	
	void update() override;
	
	void draw();
	void select(const int3 & tile);
	void erase(const int3 & tile);
	void clear();
	
	const std::set<int3> & selection() const;
	
private:
	std::set<int3> area, areaAdd, areaErase;

	void selectionMade();  //TODO: consider making it a signal
};


class TerrainLayer: public AbstractLayer
{
public:
	TerrainLayer(MapScene * s);
	
	void update() override;
	
	void draw(bool onlyDirty = true);
	void setDirty(const int3 & tile);
	
private:
	std::set<int3> dirty;
};


class ObjectsLayer: public AbstractLayer
{
public:
	ObjectsLayer(MapScene * s);
	
	void update() override;
	
	void draw(bool onlyDirty = true); //TODO: implement dirty
	
	void setDirty(int x, int y);
	void setDirty(const CGObjectInstance * object);
	
private:
	std::set<const CGObjectInstance *> dirty;
};


class SelectionObjectsLayer: public AbstractLayer
{
public:
	SelectionObjectsLayer(MapScene * s);
	
	void update() override;
	
	void draw();
	
	CGObjectInstance * selectObjectAt(int x, int y) const;
	void selectObjects(int x1, int y1, int x2, int y2);
	void selectObject(CGObjectInstance *, bool inform = true);
	bool isSelected(const CGObjectInstance *) const;
	std::set<CGObjectInstance*> getSelection() const;
	void moveSelection(int x, int y);
	void clear();
		
	QPoint shift;
	CGObjectInstance * newObject;
	int selectionMode = 0; //0 - nothing, 1 - selection, 2 - movement
	
private:
	std::set<CGObjectInstance *> selectedObjects;

	void selectionMade(); //TODO: consider making it a signal
};

#endif // SCENELAYER_H
