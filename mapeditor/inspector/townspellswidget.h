/*
 * townspellswidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QDialog>
#include "../../lib/mapObjects/CGTownInstance.h"

namespace Ui {
	class TownSpellsWidget;
}


class TownSpellsWidget : public QDialog
{
	Q_OBJECT

public:
	explicit TownSpellsWidget(CGTownInstance &, QWidget * parent = nullptr);
	~TownSpellsWidget();

	void obtainData();
	void commitChanges();

private slots:
	void on_customizeSpells_toggled(bool checked);

private:
	Ui::TownSpellsWidget * ui;

	CGTownInstance & town;

	std::array<QListWidget *, 5> possibleSpellLists;
	std::array<QListWidget *, 5> requiredSpellLists;

	void resetSpells();
	void initSpellLists();
};

class TownSpellsDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	TownSpellsDelegate(CGTownInstance&);

	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex& index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;

private:
	CGTownInstance& town;
};
