/*
 * EditorTemplateView.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../gui/CIntObject.h"

#include "../../../lib/rmg/CRmgTemplate.h"

class IImage;

/// Diagram of a random map template: the zones as cards and the connections between them as lines
class EditorTemplateView : public CIntObject
{
	static constexpr double CARD_SIZE = 130.0;

	struct CardLayout
	{
		TRmgTemplateZoneId id;
		Rect area;
	};

	std::shared_ptr<CRmgTemplate> currentTemplate;
	std::array<std::shared_ptr<IImage>, 7> resourceIcons;
	TRmgTemplateZoneId selectedZone = -1;
	int selectedConnection = -1;
	Point viewCenter;
	int zoomLevel = 0;
	double pinchFactor = 1.0;
	bool draggingCard = false;
	bool panning = false;
	Point dragStart;
	Point cardStart;

	double scale() const;
	Point toScreen(const Point & world) const;
	Point toWorld(const Point & screen) const;
	std::vector<CardLayout> cardLayouts() const;
	TRmgTemplateZoneId cardAt(const Point & cursor) const;
	int connectionAt(const Point & cursor) const;
	void drawCard(Canvas & to, const CardLayout & layout) const;
	void drawConnection(Canvas & to, const rmg::ZoneConnection & connection, bool selected) const;
	void select(TRmgTemplateZoneId zone, int connection);

public:
	/// Grid that the cards snap to
	static constexpr int GRID_SIZE = 10;

	/// Called after the user selected a zone or deselected it, with -1
	std::function<void(TRmgTemplateZoneId)> onZoneSelected;
	/// Called while the user moves a zone
	std::function<void(TRmgTemplateZoneId)> onZoneMoved;
	std::function<void(int)> onConnectionSelected;

	EditorTemplateView(const Rect & area);

	void setTemplate(const std::shared_ptr<CRmgTemplate> & newTemplate);
	/// Point of the diagram that is in the middle of the view
	Point getViewCenter() const;
	void setSelectedZone(TRmgTemplateZoneId zone);
	void setSelectedConnection(int connection);
	void zoom(int steps);
	void resetZoom();
	/// Chooses the zoom and the position so that all the zones are visible
	void autoFit();

	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickCancel(const Point & cursorPosition) override;
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void gesturePinch(const Point & centerPosition, double lastUpdateFactor) override;
	void wheelScrolled(int distance) override;
	void showAll(Canvas & to) override;
};
