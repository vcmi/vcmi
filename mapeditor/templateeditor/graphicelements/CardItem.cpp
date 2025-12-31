/*
 * CardItem.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CardItem.h"

#include <QObject>

#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/rmg/CRmgTemplate.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/ResourceTypeHandler.h"

QDomElement CardItem::getElementById(const QDomDocument& doc, const QString& id)
{
	QDomElement root = doc.documentElement();

	std::function<QDomElement(const QDomElement&)> findById = [&](const QDomElement& elem) -> QDomElement {
		if (elem.attribute("id") == id)
			return elem;

		QDomElement child = elem.firstChildElement();
		while (!child.isNull())
		{
			QDomElement found = findById(child);
			if (!found.isNull())
				return found;
			child = child.nextSiblingElement();
		}
		return QDomElement();
	};

	return findById(root);
}

bool isBlackTextNeeded(const QColor& bg)
{
	int r = bg.red();
	int g = bg.green();
	int b = bg.blue();

	double luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255;
	return luminance > 0.5;
}

CardItem::CardItem():
	selectCallback(nullptr),
	posChangeCallback(nullptr),
	useBlackText(false),
	mousePressed(false)
{
	QFile file(":/icons/templateSquare.svg");
	file.open(QIODevice::ReadOnly);
	QByteArray data = file.readAll();
	doc.setContent(data);

	updateContent();
}

void CardItem::setSelectCallback(std::function<void(bool)> func)
{
	selectCallback = func;
}

void CardItem::setPosChangeCallback(std::function<void(QPointF)> func)
{
	posChangeCallback = func;
}

void CardItem::updateContent()
{
	setSharedRenderer(new QSvgRenderer(doc.toByteArray()));
}

void CardItem::setFillColor(QColor color)
{
	auto squareElem = getElementById(doc, "rect");
	squareElem.setAttribute("style", squareElem.attribute("style").replace(QRegularExpression("fill:.*?;"), "fill:" + color.name() + ";"));

	useBlackText = isBlackTextNeeded(color);
}

void CardItem::setMultiFillColor(QColor color1, QColor color2)
{
	auto squareElem = getElementById(doc, "rect");
	squareElem.setAttribute("style", squareElem.attribute("style").replace(QRegularExpression("fill:.*?;"), "fill:url(#gradientExtra);"));
	auto gradientStopElem1 = getElementById(doc, "gradientExtraColorStop1");
	auto gradientStopElem2 = getElementById(doc, "gradientExtraColorStop2");
	auto gradientStopElem3 = getElementById(doc, "gradientExtraColorStop3");
	auto gradientStopElem4 = getElementById(doc, "gradientExtraColorStop4");
	auto gradientStopElem5 = getElementById(doc, "gradientExtraColorStop5");
	gradientStopElem1.setAttribute("style", gradientStopElem1.attribute("style").replace(QRegularExpression("stop-color:.*?;"), "stop-color:" + color1.name() + ";"));
	gradientStopElem2.setAttribute("style", gradientStopElem2.attribute("style").replace(QRegularExpression("stop-color:.*?;"), "stop-color:" + color2.name() + ";"));
	gradientStopElem3.setAttribute("style", gradientStopElem3.attribute("style").replace(QRegularExpression("stop-color:.*?;"), "stop-color:" + color1.name() + ";"));
	gradientStopElem4.setAttribute("style", gradientStopElem4.attribute("style").replace(QRegularExpression("stop-color:.*?;"), "stop-color:" + color2.name() + ";"));
	gradientStopElem5.setAttribute("style", gradientStopElem5.attribute("style").replace(QRegularExpression("stop-color:.*?;"), "stop-color:" + color1.name() + ";"));

	useBlackText = isBlackTextNeeded(color1);
}

void CardItem::setPlayerColor(PlayerColor color)
{
	std::map<PlayerColor, std::pair<QString, QString>> colors =
	{
		{ PlayerColor(0), { "#F80000", "#920000" } }, //red
		{ PlayerColor(1), { "#0000F8", "#000092" } }, //blue
		{ PlayerColor(2), { "#9B7251", "#35271C" } }, //tan
		{ PlayerColor(3), { "#00FC00", "#009600" } }, //green
		{ PlayerColor(4), { "#F88000", "#924B00" } }, //orange
		{ PlayerColor(5), { "#F800F8", "#920092" } }, //purple
		{ PlayerColor(6), { "#00FCF8", "#009694" } }, //teal
		{ PlayerColor(7), { "#C07888", "#5A3840" } }, //pink
	};
	auto squareElem = getElementById(doc, "rect");
	squareElem.setAttribute("style", squareElem.attribute("style").replace(QRegularExpression("fill:.*?;"), "fill:url(#gradientPlayer);"));
	auto gradientStopElem1 = getElementById(doc, "gradientPlayerColorStop1");
	auto gradientStopElem2 = getElementById(doc, "gradientPlayerColorStop2");
	gradientStopElem1.setAttribute("style", gradientStopElem1.attribute("style").replace(QRegularExpression("stop-color:.*?;"), "stop-color:" + colors[color].first + ";"));
	gradientStopElem2.setAttribute("style", gradientStopElem2.attribute("style").replace(QRegularExpression("stop-color:.*?;"), "stop-color:" + colors[color].second + ";"));

	useBlackText = isBlackTextNeeded(QColor(colors[color].first));
}

void CardItem::setJunction(bool val)
{
	auto squareElem = getElementById(doc, "rectJunction");
	squareElem.setAttribute("style", squareElem.attribute("style").replace(QRegularExpression("stroke-opacity:.*?;"), "stroke-opacity:" + QString::fromStdString(val ? "0.3" : "0.0") + ";"));
}

void CardItem::setId(int val)
{
	auto textIdElem = getElementById(doc, "textId");
	textIdElem.setAttribute("style", textIdElem.attribute("style").replace(QRegularExpression("fill:.*?;"), "fill:" + QColor(useBlackText ? Qt::black : Qt::white).name() + ";"));
	textIdElem.firstChild().setNodeValue(QString::number(val));

	id = val;
}

int CardItem::getId()
{
	return id;
}

void CardItem::setResAmount(GameResID res, int val)
{
	auto textElem = getElementById(doc, "text" + QString::fromStdString(res.toResource()->getJsonKey()));
	textElem.setAttribute("style", textElem.attribute("style").replace(QRegularExpression("fill:.*?;"), "fill:" + QColor(useBlackText ? Qt::black : Qt::white).name() + ";"));
	textElem.firstChild().setNodeValue(val ? QString::number(val) : "");

	auto iconElem = getElementById(doc, "icon" + QString::fromStdString(res.toResource()->getJsonKey()));
	iconElem.setAttribute("opacity", val ? "1.0" : "0.1");
}

void CardItem::setChestValue(int val)
{
	auto textElem = getElementById(doc, "textChest");
	textElem.setAttribute("style", textElem.attribute("style").replace(QRegularExpression("fill:.*?;"), "fill:" + QColor(useBlackText ? Qt::black : Qt::white).name() + ";"));
	textElem.firstChild().setNodeValue(val ? QString::number(val) : "");

	auto iconElem = getElementById(doc, "iconChest");
	iconElem.setAttribute("opacity", val ? "1.0" : "0.1");
}

void CardItem::setMonsterStrength(EMonsterStrength::EMonsterStrength val)
{
	int level = 0;
	if(val == EMonsterStrength::ZONE_WEAK || val == EMonsterStrength::GLOBAL_WEAK)
		level = 1;
	else if(val == EMonsterStrength::ZONE_NORMAL || val == EMonsterStrength::GLOBAL_NORMAL)
		level = 2;
	else if(val == EMonsterStrength::ZONE_STRONG || val == EMonsterStrength::GLOBAL_STRONG)
		level = 3;
		
	getElementById(doc, "iconSword1").setAttribute("opacity", level > 0 ? "1.0" : "0.1");
	getElementById(doc, "iconSword2").setAttribute("opacity", level > 1 ? "1.0" : "0.1");
	getElementById(doc, "iconSword3").setAttribute("opacity", level > 2 ? "1.0" : "0.1");
}

void CardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if(event->button() == Qt::LeftButton)
	{
		// set element in grid
		double xx = x() + (boundingRect().width() / 2);
		double yy = y() + (boundingRect().height() / 2);
		xx = GRID_SIZE * round(xx / GRID_SIZE);
		yy = GRID_SIZE * round(yy / GRID_SIZE);
		setPos(xx - (boundingRect().width() / 2), yy - (boundingRect().height() / 2));
	}

	QGraphicsSvgItem::mouseReleaseEvent(event);

	if(posChangeCallback)
		posChangeCallback(pos());

	mousePressed = false;
}

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsSvgItem::mousePressEvent(event);

	mousePressed = true;
}

QVariant CardItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if(change == ItemSelectedHasChanged && selectCallback)
		selectCallback(isSelected());
	else if(change == ItemPositionHasChanged && posChangeCallback && mousePressed)
		posChangeCallback(pos());

	return QGraphicsSvgItem::itemChange(change, value);
}
