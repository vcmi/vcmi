/*
 * campaignview.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "campaignview.h"

CampaignScene::CampaignScene():
	QGraphicsScene(nullptr)
{
}

CampaignView::CampaignView(QWidget * parent):
	QGraphicsView(parent)
{
}

ClickablePixmapItem::ClickablePixmapItem(const QPixmap &pixmap, const std::function<void()> & clickedCallback, const std::function<void()> & doubleClickedCallback, const std::function<void(QGraphicsSceneContextMenuEvent *)> & contextMenuCallback):
	QGraphicsPixmapItem(pixmap),
	clickedCallback(clickedCallback),
	doubleClickedCallback(doubleClickedCallback),
	contextMenuCallback(contextMenuCallback)
{
}

void ClickablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(clickedCallback)
		clickedCallback();
}

void ClickablePixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if(doubleClickedCallback)
		doubleClickedCallback();
}

void ClickablePixmapItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	if(contextMenuCallback)
		contextMenuCallback(event);
}
