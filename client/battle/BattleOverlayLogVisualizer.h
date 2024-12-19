/*
 * BattleOverlayLogVisualizer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/logging/VisualLogger.h"
#include "BattleRenderer.h"

class Canvas;
class BattleInterface;

class BattleOverlayLogVisualizer : public IBattleOverlayLogVisualizer
{
private:
	BattleRenderer::RendererRef & target;
	BattleInterface & owner;

public:
	BattleOverlayLogVisualizer(BattleRenderer::RendererRef & target, BattleInterface & owner);

	void drawText(BattleHex hex, int lineNumber, const std::string & text) override;
};
