/*
 * armywidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"
#include <QDialog>
#include "../lib/mapObjects/CArmedInstance.h"

const int TOTAL_SLOTS = 7;

namespace Ui {
class ArmyWidget;
}

class ArmyWidget : public QDialog
{
	Q_OBJECT

public:
	explicit ArmyWidget(CArmedInstance &, QWidget *parent = nullptr);
	~ArmyWidget();
	
	void obtainData();
	bool commitChanges();

private:
	int searchItemIndex(int slotId, CreatureID creId) const;
	
	Ui::ArmyWidget *ui;
	CArmedInstance & army;
	std::array<QLineEdit*, TOTAL_SLOTS> uiCounts{};
	std::array<QComboBox*, TOTAL_SLOTS> uiSlots{};
};

class ArmyDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	
	ArmyDelegate(CArmedInstance &);
	
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	
private:
	CArmedInstance & army;
};

