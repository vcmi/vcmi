/*
 * scholarwidget.h, part of VCMI engine
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
#include "lib/json/JsonKeyExtractor.h"
#include "lib/mapObjects/CRewardableObject.h"

VCMI_LIB_USING_NAMESPACE

namespace Ui
{
class ScholarWidget;
}

class MapController;
class CRewardableObject;

class ScholarWidget : public QDialog
{
	Q_OBJECT

public:
	explicit ScholarWidget(CRewardableObject &, MapController &, QWidget * parent = nullptr);
	~ScholarWidget();
	void loadData();
	void commitChanges();

private slots:
	void on_pSkillsCheck_toggled(bool checked);
	void on_sSkillsCheck_toggled(bool checked);
	void on_spellsCheck_toggled(bool checked);

private:
	struct RewardData
	{
		QRadioButton * radioButton;
		QComboBox * comboBox;
		std::array<std::string, 2> variables;
		std::string name;
		JsonNode dice;
	};

	void loadState();
	void changeComboBoxesAllowedState();
	void showInvalidPresetWarning(std::string type, std::string name);

	Ui::ScholarWidget * ui;
	CRewardableObject & scholar;
	JsonKeyExtractor extractor;
	MapController & controller;

	std::vector<RewardData> rewardsData;
	static constexpr const char * presetNotFoundWarning =
		"<font color='red'>The scholar has %1 preset set to \"%2\", "
		"but the value is unknown. Maybe it is a mod configuration problem?</font color='red'>";
};

class ScholarDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	ScholarDelegate(MapController &, CRewardableObject &);

	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;
	void addEntityName(QStringList & textList, const Entity * const entity) const;

private:
	CRewardableObject & scholar;
	MapController & controller;
};
