/*
 * templateview.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QGraphicsView>
#include <QGraphicsPixmapItem>

class TemplateScene : public QGraphicsScene
{
	Q_OBJECT;
public:
	TemplateScene();
};

class TemplateView : public QGraphicsView
{
	Q_OBJECT

	int zoomlevel = 0;

public:
	TemplateView(QWidget * parent);

	void setZoomLevel(int level);
	void changeZoomLevel(bool increase);
	void autoFit();

	void wheelEvent(QWheelEvent * e) override;
};
