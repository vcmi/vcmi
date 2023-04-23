/*
 * CAdvMapPanel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CAdvMapPanel.h"

#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../gui/CGuiHandler.h"

CAdvMapPanel::CAdvMapPanel(std::shared_ptr<IImage> bg, Point position):
	background(bg)
{
	defActions = 255;
	recActions = 255;
	pos.x += position.x;
	pos.y += position.y;
	if (bg)
	{
		pos.w = bg->width();
		pos.h = bg->height();
	}
}

void CAdvMapPanel::addChildColorableButton(std::shared_ptr<CButton> button)
{
	colorableButtons.push_back(button);
	addChildToPanel(button, ACTIVATE | DEACTIVATE);
}

void CAdvMapPanel::setPlayerColor(const PlayerColor & clr)
{
	for(auto & button : colorableButtons)
	{
		button->setPlayerColor(clr);
	}
}

void CAdvMapPanel::showAll(SDL_Surface * to)
{
	if(background)
		background->draw(to, pos.x, pos.y);

	CIntObject::showAll(to);
}

void CAdvMapPanel::addChildToPanel(std::shared_ptr<CIntObject> obj, ui8 actions)
{
	otherObjects.push_back(obj);
	obj->recActions |= actions | SHOWALL;
	obj->recActions &= ~DISPOSE;
	addChild(obj.get(), false);
}

CAdvMapWorldViewPanel::CAdvMapWorldViewPanel(std::shared_ptr<CAnimation> _icons, std::shared_ptr<IImage> bg, Point position, int spaceBottom, const PlayerColor &color)
	: CAdvMapPanel(bg, position), icons(_icons)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	int fillerHeight = bg ? spaceBottom - pos.y - pos.h : 0;

	if(fillerHeight > 0)
	{
		backgroundFiller = std::make_shared<CFilledTexture>("DIBOXBCK", Rect(0, pos.h, pos.w, fillerHeight));
	}
}

void CAdvMapWorldViewPanel::recolorIcons(const PlayerColor & color, int indexOffset)
{
	assert(iconsData.size() == currentIcons.size());

	for(size_t idx = 0; idx < iconsData.size(); idx++)
	{
		const auto & data = iconsData.at(idx);
		currentIcons[idx]->setFrame(data.first + indexOffset);
	}
}

void CAdvMapWorldViewPanel::addChildIcon(std::pair<int, Point> data, int indexOffset)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	iconsData.push_back(data);
	currentIcons.push_back(std::make_shared<CAnimImage>(icons, data.first + indexOffset, 0, data.second.x, data.second.y));
}

