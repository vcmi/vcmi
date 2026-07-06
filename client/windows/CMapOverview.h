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

class CMap;
#include "CWindowObject.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../gui/InterfaceObjectConfigurable.h"

class CSlider;
class CLabel;
class CPicture;
class CFilledTexture;
class CTextBox;
class IImage;
class CanvasImage;
class TransparentFilledRectangle;
enum class ESelectionScreen : ui8;

class CMapOverview;

class CMapOverviewWidget : public InterfaceObjectConfigurable
{
	CMapOverview& p;

	bool drawPlayerElements;
	std::vector<std::shared_ptr<CanvasImage>> minimaps;

	std::shared_ptr<CanvasImage> createMinimapForLayer(std::unique_ptr<CMap> & map, int layer) const;
	std::vector<std::shared_ptr<CanvasImage>> createMinimaps(const ResourcePath & resource) const;
	std::vector<std::shared_ptr<CanvasImage>> createMinimaps(std::unique_ptr<CMap> & map) const;

	void resizeMinimaps(int size) const;
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
	const std::string author;
	const std::string version;
	const ESelectionScreen tabType;

	CMapOverview(const std::string & mapName, const std::string & fileName, const std::string & date, const std::string & author, const std::string & version, const ResourcePath & resource, ESelectionScreen tabType);
};
