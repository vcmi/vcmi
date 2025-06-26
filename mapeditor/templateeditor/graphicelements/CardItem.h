/*
 * CardItem.h, part of VCMI engine
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
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/rmg/CRmgTemplate.h"

class CardItem : public QGraphicsSvgItem
{
private:
	QDomElement getElementById(const QDomDocument& doc, const QString& id);

	QDomDocument doc;
	bool useBlackText;
	int id = -1;
	bool mousePressed;

	std::function<void(bool)> selectCallback;
	std::function<void(QPointF)> posChangeCallback;
public:
	CardItem();

	static constexpr double GRID_SIZE = 10.;

	void setSelectCallback(std::function<void(bool)> func);
	void setPosChangeCallback(std::function<void(QPointF)> func);

	void updateContent();
	void setFillColor(QColor color);
	void setMultiFillColor(QColor color1, QColor color2);
	void setPlayerColor(PlayerColor color);
	void setJunction(bool val);
	void setId(int val);
	int getId();
	void setResAmount(GameResID res, int val);
	void setChestValue(int val);
	void setMonsterStrength(EMonsterStrength::EMonsterStrength val);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};
