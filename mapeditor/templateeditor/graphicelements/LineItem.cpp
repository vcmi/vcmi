/*
 * LineItem.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LineItem.h"

#include <QObject>

LineItem::LineItem():
	clickCallback(nullptr)
{
	setZValue(-2);
	for(int i = 0; i < 10; i++) // render multiple times to increase outline effect
	{
		auto tmpTextItem = new QGraphicsTextItem(this);
		tmpTextItem->setZValue(-1);
		QFont font;
		font.setPointSize(18);
		tmpTextItem->setFont(font);
		QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect();
		shadowEffect->setBlurRadius(10);
		shadowEffect->setEnabled(true);
		shadowEffect->setOffset(0, 0);
		shadowEffect->setColor(Qt::black);
		tmpTextItem->setGraphicsEffect(shadowEffect);
		tmpTextItem->setDefaultTextColor(Qt::white);
		textItem.push_back(tmpTextItem);
	}
}

void LineItem::setLineToolTip(const QString &toolTip)
{
	for(auto & tmpTextItem : textItem)
		tmpTextItem->setToolTip(toolTip);
	setToolTip(toolTip);
}

void LineItem::setClickCallback(std::function<void()> func)
{
	clickCallback = func;
}

void LineItem::setText(QString text)
{
	for(auto & tmpTextItem : textItem)
	{
		tmpTextItem->setPlainText(text);
		QRectF lineRect = boundingRect();
		QRectF textRect = tmpTextItem->boundingRect();
		tmpTextItem->setPos(QPointF(lineRect.x() + (lineRect.width() / 2) - (textRect.width() / 2), lineRect.y() + (lineRect.height() / 2) - (textRect.height() / 2)));
	}
}

void LineItem::setId(int val)
{
	id = val;
}

int LineItem::getId()
{
	return id;
}

void LineItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(event->button() == Qt::LeftButton && clickCallback)
		clickCallback();

	QGraphicsLineItem::mousePressEvent(event);
}
