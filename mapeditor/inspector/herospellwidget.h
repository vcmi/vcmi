/*
 * herospellwidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QDialog>
#include "baseinspectoritemdelegate.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

namespace Ui {
	class HeroSpellWidget;
}


class HeroSpellWidget : public QDialog
{
	Q_OBJECT

public:
	explicit HeroSpellWidget(CGHeroInstance &, QWidget * parent = nullptr);
	~HeroSpellWidget();

	void obtainData();
	void commitChanges();

private slots:
	void on_customizeSpells_toggled(bool checked);
	void on_filter_textChanged(const QString & keyword);

private:
	Ui::HeroSpellWidget * ui;

	CGHeroInstance & hero;

	void initSpellLists();
	void showItemIfMatches(const QString & match);
	void hideItemIfMatches(const QString & match);
	void toggleHiddenForItemIfMatches(const QString & match, bool hidden);
	void hideEmptySpellLists();
};

class HeroSpellDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	HeroSpellDelegate(CGHeroInstance &);

	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex& index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;

private:
	CGHeroInstance & hero;
};
