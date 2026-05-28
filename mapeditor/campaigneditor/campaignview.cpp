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
	setAcceptDrops(true);
}

void CampaignView::dragEnterEvent(QDragEnterEvent *event)
{
	if(event->mimeData()->hasUrls())
	{
		for(const QUrl& url : event->mimeData()->urls())
		{
			QString path = url.toLocalFile();
			if(path.endsWith(".h3c", Qt::CaseInsensitive) || path.endsWith(".vcmp", Qt::CaseInsensitive))
			{
				event->acceptProposedAction();
				return;
			}
		}
	}
}

void CampaignView::dragMoveEvent(QDragMoveEvent *event)
{
	if(event->mimeData()->hasUrls())
	{
		for(const QUrl& url : event->mimeData()->urls())
		{
			QString path = url.toLocalFile();
			if(path.endsWith(".h3c", Qt::CaseInsensitive) || path.endsWith(".vcmp", Qt::CaseInsensitive))
			{
				event->acceptProposedAction();
				return;
			}
		}
	}
}

void CampaignView::dropEvent(QDropEvent *event)
{
	if(event->mimeData()->hasUrls())
	{
		for(const QUrl& url : event->mimeData()->urls())
		{
			QString path = url.toLocalFile();
			if(path.endsWith(".h3c", Qt::CaseInsensitive) || path.endsWith(".vcmp", Qt::CaseInsensitive))
			{
				Q_EMIT fileDropped(path);
				event->acceptProposedAction();
				return;
			}
		}
	}
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
