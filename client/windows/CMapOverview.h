/*
 * CMapOverview.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class CMap;
VCMI_LIB_NAMESPACE_END
#include "CWindowObject.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../gui/InterfaceObjectConfigurable.h"

class CSlider;
class CLabel;
class CPicture;
class CFilledTexture;
class CTextBox;
class IImage;
class Canvas;
class TransparentFilledRectangle;
enum class ESelectionScreen : ui8;

class CMapOverview;

class CMapOverviewWidget : public InterfaceObjectConfigurable
{
	CMapOverview& p;

	bool drawPlayerElements;
	std::vector<Canvas> minimaps;

	Canvas createMinimapForLayer(std::unique_ptr<CMap> & map, int layer) const;
	std::vector<Canvas> createMinimaps(ResourcePath resource) const;
	std::vector<Canvas> createMinimaps(std::unique_ptr<CMap> & map) const;

	std::shared_ptr<CPicture> buildDrawMinimap(const JsonNode & config) const;
public:
	CMapOverviewWidget(CMapOverview& p);
};

class CMapOverview : public CWindowObject
{
	std::shared_ptr<CMapOverviewWidget> widget;

public:
	const ResourcePath resource;
	const std::string mapName;
	const std::string fileName;
	const std::string date;
	const ESelectionScreen tabType;

	CMapOverview(std::string mapName, std::string fileName, std::string date, ResourcePath resource, ESelectionScreen tabType);
};
