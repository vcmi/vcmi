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
#include "lib/modding/ModVerificationInfo.h"
#include "../lib/callback/EditorCallback.h"

VCMI_LIB_NAMESPACE_BEGIN
using ModCompatibilityInfo = std::map<std::string, ModVerificationInfo>;
class EditorObstaclePlacer;
VCMI_LIB_NAMESPACE_END

class MainWindow;
class MapController : public QObject
{
	Q_OBJECT

public:
	explicit MapController(QObject * parent = nullptr);
	MapController(MainWindow *);
	MapController(const MapController &) = delete;
	MapController(const MapController &&) = delete;
	~MapController();
	
	void setCallback(std::unique_ptr<EditorCallback>);
	EditorCallback * getCallback();
	void setMap(std::unique_ptr<CMap>);
	void initObstaclePainters(CMap * map);
	
	static void repairMap(CMap * map);
	void repairMap();
	
	const std::unique_ptr<CMap> & getMapUniquePtr() const; //to be used for map saving
	CMap * map();
	MapHandler * mapHandler();
	MapScene * scene(int level);
	std::set<MapScene *> getScenes();
	MinimapScene * miniScene(int level);
	
	void resetMapHandler();
	
	void initializeMap();
	void sceneForceUpdate();
	
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
	bool canPlaceObject(const CGObjectInstance * obj, QString & error) const;
	bool canPlaceGrail(const CGObjectInstance * grailObj, QString & error) const;
	bool canPlaceHero(const CGObjectInstance * heroObj, QString & error) const;
	
	/// Ensures that the object's mod is listed in the map's required mods.
	/// If the mod is missing, prompts the user to add it. Returns false if the user declines,
	/// making the object invalid for placement.
	bool checkRequiredMods(const CGObjectInstance * obj, QString & error) const;

	/// These functions collect mod verification data for gameplay objects by scanning map objects
	/// and their nested elements (like spells and artifacts). The gathered information
	/// is used to assess compatibility and integrity of mods used in a given map or game state
	static void modAssessmentObject(const CGObjectInstance * obj, ModCompatibilityInfo & result);
	static ModCompatibilityInfo modAssessmentAll();
	static ModCompatibilityInfo modAssessmentMap(const CMap & map);

	/// Returns formatted message string describing a missing mod requirement for the map.
	/// Used in both warnings and confirmations related to required mod dependencies.
	static QString modMissingMessage(const ModVerificationInfo & info);

	void undo();
	void redo();
	
	PlayerColor defaultPlayer;
	QDialog * settingsDialog = nullptr;

signals:
	void requestModsUpdate(const ModCompatibilityInfo & mods, bool leaveCheckedUnchanged) const;
	
private:
	std::unique_ptr<EditorCallback> _cb;
	std::unique_ptr<CMap> _map;
	std::unique_ptr<MapHandler> _mapHandler;
	MainWindow * main;
	mutable std::map<int, std::unique_ptr<MapScene>> _scenes;
	mutable std::map<int, std::unique_ptr<MinimapScene>> _miniscenes;
	std::vector<std::unique_ptr<CGObjectInstance>> _clipboard;
	int _clipboardShiftIndex = 0;

	const int MAX_LEVELS = 10; // TODO: multilevel support: remove this constant

	std::map<TerrainId, std::unique_ptr<EditorObstaclePlacer>> _obstaclePainters;

	void connectScenes();
};
