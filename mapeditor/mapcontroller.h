/*
 * mapcontroller.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "maphandler.h"
#include "mapview.h"

VCMI_LIB_NAMESPACE_BEGIN
struct ModVerificationInfo;
using ModCompatibilityInfo = std::map<std::string, ModVerificationInfo>;
class EditorObstaclePlacer;
VCMI_LIB_NAMESPACE_END

class MainWindow;
class MapController
{
public:
	MapController(MainWindow *);
	MapController(const MapController &) = delete;
	MapController(const MapController &&) = delete;
	~MapController();
	
	void setMap(std::unique_ptr<CMap>);
	void initObstaclePainters(CMap * map);
	
	static void repairMap(CMap * map);
	void repairMap();
	
	const std::unique_ptr<CMap> & getMapUniquePtr() const; //to be used for map saving
	CMap * map();
	MapHandler * mapHandler();
	MapScene * scene(int level);
	MinimapScene * miniScene(int level);
	
	void resetMapHandler();
	
	void sceneForceUpdate();
	void sceneForceUpdate(int level);
	
	void commitTerrainChange(int level, const TerrainId & terrain);
	void commitRoadOrRiverChange(int level, ui8 type, bool isRoad);
	void commitObjectErase(const CGObjectInstance* obj);
	void commitObjectErase(int level);
	void commitObstacleFill(int level);
	void commitChangeWithoutRedraw();
	void commitObjectShift(int level);
	void commitObjectCreate(int level);
	void commitObjectChange(int level);
	
	void copyToClipboard(int level);
	void pasteFromClipboard(int level);
	
	bool discardObject(int level) const;
	void createObject(int level, std::shared_ptr<CGObjectInstance> obj) const;
	bool canPlaceObject(int level, CGObjectInstance * obj, QString & error) const;
	
	static ModCompatibilityInfo modAssessmentAll();
	static ModCompatibilityInfo modAssessmentMap(const CMap & map);

	void undo();
	void redo();
	
	PlayerColor defaultPlayer;
	QDialog * settingsDialog = nullptr;
	
private:
	std::unique_ptr<CMap> _map;
	std::unique_ptr<MapHandler> _mapHandler;
	MainWindow * main;
	mutable std::array<std::unique_ptr<MapScene>, 2> _scenes;
	mutable std::array<std::unique_ptr<MinimapScene>, 2> _miniscenes;
	std::vector<std::unique_ptr<CGObjectInstance>> _clipboard;
	int _clipboardShiftIndex = 0;

	std::map<TerrainId, std::unique_ptr<EditorObstaclePlacer>> _obstaclePainters;

	void connectScenes();
};
