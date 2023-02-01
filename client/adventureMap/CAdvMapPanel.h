/*
 * AdventureMapClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ObjectLists.h"
#include "../../lib/FunctionList.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class CGGarrison;
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
struct Component;
struct InfoAboutArmy;
struct InfoAboutHero;
struct InfoAboutTown;

VCMI_LIB_NAMESPACE_END

class CAnimation;
class CAnimImage;
class CShowableAnim;
class CFilledTexture;
class CButton;
class CComponent;
class CHeroTooltip;
class CTownTooltip;
class CTextBox;
class IImage;

/// simple panel that contains other displayable elements; used to separate groups of controls
class CAdvMapPanel : public CIntObject
{
	std::vector<std::shared_ptr<CButton>> colorableButtons;
	std::vector<std::shared_ptr<CIntObject>> otherObjects;
	/// the surface passed to this obj will be freed in dtor
	std::shared_ptr<IImage> background;
public:
	CAdvMapPanel(std::shared_ptr<IImage> bg, Point position);

	void addChildToPanel(std::shared_ptr<CIntObject> obj, ui8 actions = 0);
	void addChildColorableButton(std::shared_ptr<CButton> button);
	/// recolors all buttons to given player color
	void setPlayerColor(const PlayerColor & clr);

	void showAll(SDL_Surface * to) override;
};

/// specialized version of CAdvMapPanel that handles recolorable def-based pictures for world view info panel
class CAdvMapWorldViewPanel : public CAdvMapPanel
{
	/// data that allows reconstruction of panel info icons
	std::vector<std::pair<int, Point>> iconsData;
	/// ptrs to child-pictures constructed from iconsData
	std::vector<std::shared_ptr<CAnimImage>> currentIcons;
	/// surface drawn below world view panel on higher resolutions (won't be needed when world view panel is configured for extraResolutions mod)
	std::shared_ptr<CFilledTexture> backgroundFiller;
	std::shared_ptr<CAnimation> icons;
public:
	CAdvMapWorldViewPanel(std::shared_ptr<CAnimation> _icons, std::shared_ptr<IImage> bg, Point position, int spaceBottom, const PlayerColor &color);
	virtual ~CAdvMapWorldViewPanel();

	void addChildIcon(std::pair<int, Point> data, int indexOffset);
	/// recreates all pictures from given def to recolor them according to current player color
	void recolorIcons(const PlayerColor & color, int indexOffset);
};

