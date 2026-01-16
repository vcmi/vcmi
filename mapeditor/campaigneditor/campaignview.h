/*
 * campaignview.h, part of VCMI engine
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

class CampaignScene : public QGraphicsScene
{
	Q_OBJECT;
public:
	CampaignScene();
};

class CampaignView : public QGraphicsView
{
	Q_OBJECT

public:
	CampaignView(QWidget * parent);

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;

signals:
	void fileDropped(const QString & filename);
};

class ClickablePixmapItem : public QGraphicsPixmapItem {
public:
	ClickablePixmapItem(const QPixmap &pixmap, const std::function<void()> & clickedCallback, const std::function<void()> & doubleClickedCallback, const std::function<void(QGraphicsSceneContextMenuEvent *)> & contextMenuCallback);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
	std::function<void()> clickedCallback;
	std::function<void()> doubleClickedCallback;
	std::function<void(QGraphicsSceneContextMenuEvent *)> contextMenuCallback;
};