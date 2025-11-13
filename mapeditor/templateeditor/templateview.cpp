/*
 * templateview.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "templateview.h"

TemplateScene::TemplateScene():
	QGraphicsScene(nullptr)
{
}

TemplateView::TemplateView(QWidget * parent):
	QGraphicsView(parent)
{
}

void TemplateView::setZoomLevel(int level)
{
	zoomlevel = level;

	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	float scale = pow(1.1, zoomlevel);
	QTransform matrix;
	matrix.scale(scale, scale);
	setTransform(matrix);
}

void TemplateView::changeZoomLevel(bool increase)
{
	if(increase)
		zoomlevel++;
	else
		zoomlevel--;
	
	setZoomLevel(zoomlevel);
}

void TemplateView::autoFit()
{
	fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
	
	zoomlevel = log(transform().m11()) / log(1.1f);
}

void TemplateView::wheelEvent(QWheelEvent *e)
{
	if (e->angleDelta().y() > 0)
		changeZoomLevel(true);
	else
		changeZoomLevel(false);

	e->accept();
}
