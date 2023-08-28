/*
 * TurnTimerWidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../gui/CIntObject.h"
#include "../gui/InterfaceObjectConfigurable.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"

class CAnimImage;
class CLabel;

VCMI_LIB_NAMESPACE_BEGIN

class PlayerColor;

VCMI_LIB_NAMESPACE_END

class TurnTimerWidget : public InterfaceObjectConfigurable
{
private:
	
	class DrawRect : public CIntObject
	{
		const Rect rect;
		const ColorRGBA color;
		
	public:
		DrawRect(const Rect &, const ColorRGBA &);
		void showAll(Canvas & to) override;
	};

	int turnTime;
	int lastTurnTime;
	int cachedTurnTime;
	PlayerColor lastPlayer;
	
	std::set<int> notifications;
	
	std::shared_ptr<DrawRect> buildDrawRect(const JsonNode & config) const;

public:

	void show(Canvas & to) override;
	void tick(uint32_t msPassed) override;
	
	void setTime(PlayerColor player, int time);

	TurnTimerWidget();
};
