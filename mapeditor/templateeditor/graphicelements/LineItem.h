/*
 * LineItem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>
#include <QGraphicsSvgItem>

#include "../../StdInc.h"

class LineItem : public QGraphicsLineItem
{
private:
	std::vector<QGraphicsTextItem *> textItem;
	std::function<void()> clickCallback;
	int id = -1;

	static constexpr int CLICKABLE_PADDING_AROUND_LINE = 10;
public:
	LineItem();
	void setClickCallback(std::function<void()> func);
	void setText(QString text);
	void setId(int val);
	int getId();
	void setLineToolTip(const QString &toolTip);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
};
