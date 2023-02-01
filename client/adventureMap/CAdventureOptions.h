/*
 * CAdventureOptions.h, part of VCMI engine
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


/// Adventure options dialog where you can view the world, dig, play the replay of the last turn,...
class CAdventureOptions : public CWindowObject
{
public:
	std::shared_ptr<CButton> exit;
	std::shared_ptr<CButton> viewWorld;
	std::shared_ptr<CButton> puzzle;
	std::shared_ptr<CButton> dig;
	std::shared_ptr<CButton> scenInfo;
	/*std::shared_ptr<CButton> replay*/

	CAdventureOptions();
	static void showScenarioInfo();
};

