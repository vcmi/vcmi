/*
 * questwidget.h, part of VCMI engine
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
#include "../lib/mapObjects/CQuest.h"
#include "../lib/mapping/CMap.h"

namespace Ui {
class QuestWidget;
}

class QuestWidget : public QDialog
{
	Q_OBJECT

public:
	explicit QuestWidget(const CMap &, CGSeerHut &, QWidget *parent = nullptr);
	~QuestWidget();
	
	void obtainData();
	QString commitChanges();

private:
	CGSeerHut & seerhut;
	const CMap & map;
	Ui::QuestWidget *ui;
};

class QuestDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	
	QuestDelegate(const CMap &, CGSeerHut &);
	
	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	
private:
	CGSeerHut & seerhut;
	const CMap & map;
};
