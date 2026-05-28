/*
 * abilitieswidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QDialog>
#include <json/JsonKeyExtractor.h>
#include "baseinspectoritemdelegate.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include <../lib/mapObjects/CRewardableObject.h>

namespace Ui
{
class AbilitiesWidget;
}

class MapController;

class AbilitiesWidget : public QDialog
{
	Q_OBJECT

public:
	explicit AbilitiesWidget(CRewardableObject &, MapController &, QWidget * parent = nullptr);
	~AbilitiesWidget();
	void loadData();
	void commitChanges();

	static const std::string V_CATEGORY;
	static const std::string V_NAME;

private slots:
	void on_selectAll_clicked();
	void on_deselectAll_clicked();
	void on_filter_textChanged(QString text);
	void on_customize_toggled(bool checked);

private:
	void prepareDisplay();
	std::set<SecondarySkill> getPresetAbilities();
	std::set<SecondarySkill> getDefaultAbilities();
	void changeCheckStateForAll(bool);
	bool isSetToDefault();

	Ui::AbilitiesWidget * ui;
	CRewardableObject & hut;
	JsonKeyExtractor extractor;
	MapController & controller;
};

class AbilitiesDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	AbilitiesDelegate(MapController &, CRewardableObject &);

	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;

private:
	CRewardableObject & hut;
	MapController & controller;
};
