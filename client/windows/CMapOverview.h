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
enum ESelectionScreen : ui8;

class CMapOverview : public CWindowObject
{
	//const int IMAGE_SIZE = 169;
	//const int BORDER = 30;

	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CTextBox> label;
	std::shared_ptr<CPicture> image1;
	std::shared_ptr<CPicture> image2;

	class CMapOverviewWidget : public InterfaceObjectConfigurable
	{
		ResourcePath resource;

		bool drawPlayerElements;
		bool renderImage;
		Canvas createMinimapForLayer(std::unique_ptr<CMap> & map, int layer) const;
		std::vector<std::shared_ptr<IImage>> createMinimaps(ResourcePath resource, Point size) const;

		std::shared_ptr<TransparentFilledRectangle> buildDrawTransparentRect(const JsonNode & config) const;
		std::shared_ptr<CPicture> buildDrawMinimap(const JsonNode & config) const;
	public:
		CMapOverviewWidget(std::string text, ResourcePath resource, ESelectionScreen tabType);
	};

	std::shared_ptr<CMapOverviewWidget> widget;

public:
	CMapOverview(std::string text, ResourcePath resource, ESelectionScreen tabType);
};