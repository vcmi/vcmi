/*
 * CIntObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CIntObject.h"
#include "GuiController.h"
#include "../lib/GameConstants.h"
#include "../lib/ResourceSet.h"
#include "../lib/CConfigHandler.h"

class UiDataBinder
{
private:
	std::map<std::string, std::function<void(const std::vector<std::string> & args)>> handlers;

public:
	UiDataBinder(IUiController & controller);

	std::function<void()> bindHandler(JsonNode & handlerNode);
};

class IControlFactory
{
public:
	virtual std::shared_ptr<CIntObject> createControl(Rect viewport, JsonNode & node, UiDataBinder & dataBinder) const = 0;

protected:
	static Rect getControlRect(Rect viewport, Point contentSize, JsonNode & node);
};

class GuiJsonLoader
{
public:
	std::shared_ptr<CIntObject> loadJson(Rect viewport, ResourceID res, IUiController & controller);
};