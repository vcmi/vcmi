/*
 * CAdventureMapInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
class IShipyard;
struct CGPathNode;
struct ObjectPosInfo;
struct Component;
class int3;

VCMI_LIB_NAMESPACE_END

class CButton;
class IImage;
class CAnimImage;
class CGStatusBar;
class CAdventureMapWidget;
class AdventureMapShortcuts;
class CAnimation;
class MapView;
class CResDataBar;
class CHeroList;
class CTownList;
class CInfoBar;
class CMinimap;
class MapAudioPlayer;

struct MapDrawingInfo;

/// That's a huge class which handles general adventure map actions and
/// shows the right menu(questlog, spellbook, end turn,..) from where you
/// can get to the towns and heroes.
class CAdventureMapInterface : public CIntObject
{
private:
	/// currently acting player
	PlayerColor currentPlayerID;

	/// uses EDirections enum
	bool scrollingCursorSet;

	const CSpell *spellBeingCasted; //nullptr if none

	std::shared_ptr<MapAudioPlayer> mapAudio;
	std::shared_ptr<CAdventureMapWidget> widget;
	std::shared_ptr<AdventureMapShortcuts> shortcuts;

private:
	bool isActive();
	void adjustActiveness(bool aiTurnStart); //should be called every time at AI/human turn transition; blocks GUI during AI turn

	const IShipyard * ourInaccessibleShipyard(const CGObjectInstance *obj) const; //checks if obj is our ashipyard and cursor is 0,0 -> returns shipyard or nullptr else

	void handleMapScrollingUpdate();

	void showMoveDetailsInStatusbar(const CGHeroInstance & hero, const CGPathNode & pathNode);

	const CGObjectInstance *getActiveObject(const int3 &tile);

	/// exits currently opened world view mode and returns to normal map
	void exitCastingMode();
	void performSpellcasting(const int3 & castTarget);

protected:
	// CIntObject interface implementation

	void activate() override;
	void deactivate() override;

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;

	void keyPressed(EShortcut key) override;

	void onScreenResize() override;

public:
	CAdventureMapInterface();

	void hotkeyAbortCastingMode();
	void hotkeyExitWorldView();
	void hotkeyEndingTurn();
	void hotkeyNextTown();
	void hotkeySwitchMapLevel();

	/// Called by PlayerInterface when specified player is ready to start his turn
	void onHotseatWaitStarted(PlayerColor playerID);

	/// Called by PlayerInterface when AI or remote human player starts his turn
	void onEnemyTurnStarted(PlayerColor playerID);

	/// Called by PlayerInterface when local human player starts his turn
	void onPlayerTurnStarted(PlayerColor playerID);

	/// Called by PlayerInterface when interface should be switched to specified player without starting turn
	void onCurrentPlayerChanged(PlayerColor playerID);

	/// Called by PlayerInterface when specific map tile changed and must be updated on minimap
	void onMapTilesChanged(boost::optional<std::unordered_set<int3>> positions);

	/// Called by PlayerInterface when hero starts movement
	void onHeroMovementStarted(const CGHeroInstance * hero);

	/// Called by PlayerInterface when hero state changed and hero list must be updated
	void onHeroChanged(const CGHeroInstance * hero);

	/// Called by PlayerInterface when town state changed and town list must be updated
	void onTownChanged(const CGTownInstance * town);

	/// Called when currently selected object changes
	void onSelectionChanged(const CArmedInstance *sel);

	/// Called when map audio should be paused, e.g. on combat or town screen access
	void onAudioPaused();

	/// Called when map audio should be resume, opposite to onPaused
	void onAudioResumed();

	/// Requests to display provided information inside infobox
	void showInfoBoxMessage(const std::vector<Component> & components, std::string message, int timer);

	/// Changes position on map to center selected location
	void centerOnTile(int3 on);
	void centerOnObject(const CGObjectInstance *obj);

	/// called by MapView whenever currently visible area changes
	/// visibleArea describes now visible map section measured in tiles
	void onMapViewMoved(const Rect & visibleArea, int mapLevel);

	/// called by MapView whenever tile is clicked
	void onTileLeftClicked(const int3 & mapPos);

	/// called by MapView whenever tile is hovered
	void onTileHovered(const int3 & mapPos);

	/// called by MapView whenever tile is clicked
	void onTileRightClicked(const int3 & mapPos);

	/// called by spell window when spell to cast has been selected
	void enterCastingMode(const CSpell * sp);

	/// returns area of screen covered by terrain (main game area)
	Rect terrainAreaPixels() const;

	/// opens world view at default scale
	void openWorldView();

	/// opens world view at specific scale
	void openWorldView(int tileSize);

	/// opens world view with specific info, e.g. after View Earth/Air is shown
	void openWorldView(const std::vector<ObjectPosInfo>& objectPositions, bool showTerrain);
};

extern std::shared_ptr<CAdventureMapInterface> adventureInt;
