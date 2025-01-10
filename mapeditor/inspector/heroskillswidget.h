/*
 * heroskillswidget.h, part of VCMI engine
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
class HeroSkillsWidget;
}

class HeroSkillsWidget : public QDialog
{
	Q_OBJECT

public:
	explicit HeroSkillsWidget(CGHeroInstance &, QWidget *parent = nullptr);
	~HeroSkillsWidget();
	
	void obtainData();
	void commitChanges();

private slots:
	void on_addButton_clicked();

	void on_removeButton_clicked();

	void on_checkBox_toggled(bool checked);

private:
	Ui::HeroSkillsWidget *ui;
	
	CGHeroInstance & hero;
	
	std::set<int> occupiedSkills;
};

class HeroSkillsDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	HeroSkillsDelegate(CGHeroInstance &);
	
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;
	
private:
	CGHeroInstance & hero;
};
