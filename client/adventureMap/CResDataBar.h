/*
 * CAdvmapInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/AdventureMapClasses.h"
#include "CWindowObject.h"

#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"

#include "../../lib/spells/ViewSpellInt.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CGPath;
struct CGPathNode;
class CGHeroInstance;
class CGTownInstance;
class CSpell;
class IShipyard;

VCMI_LIB_NAMESPACE_END

class CCallback;
class CAdvMapInt;
class CHeroWindow;
enum class EMapAnimRedrawStatus;
class CFadeAnimation;

struct MapDrawingInfo;

/// Resources bar which shows information about how many gold, crystals,... you have
/// Current date is displayed too
class CResDataBar : public CIntObject
{
public:
	std::shared_ptr<CPicture> background;

	std::vector<std::pair<int,int> > txtpos;
	std::string datetext;

	void clickRight(tribool down, bool previousState) override;
	CResDataBar();
	CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist);
	~CResDataBar();

	void draw(SDL_Surface * to);
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};

