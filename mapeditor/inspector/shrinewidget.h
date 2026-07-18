/*
 * shrinewidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "baseinspectoritemdelegate.h"
#include "lib/json/JsonKeyExtractor.h"
#include "lib/mapObjects/CRewardableObject.h"
#include <QDialog>

namespace Ui
{
class ShrineWidget;
}

class MapController;
class CRewardableObject;

class ShrineWidget : public QDialog
{
	Q_OBJECT

public:
	explicit ShrineWidget(CRewardableObject & shrine, MapController & controller, QWidget * parent = nullptr);
	~ShrineWidget();
	void loadData();
	void commitChanges();

private slots:
	void on_custom_toggled(bool checked);
	void on_random_toggled(bool checked);

private:
	si64 getSpellLevel();
	void loadState();
	void changeComboBoxAllowedState();
	void showInvalidPresetWarning(std::string name);

	Ui::ShrineWidget * ui;
	CRewardableObject & shrine;
	JsonKeyExtractor extractor;
	MapController & controller;

	static constexpr const char * PRESET_CATEGORY = "spell";
	static constexpr const char * PRESET_NAME = "gainedSpell";
	static constexpr const char * presetNotFoundWarning =
		"<font color='red'>The shrine has spell preset set to \"%1\", "
		"but the value is unknown. Maybe it is a mod configuration problem?</font color='red'>";

	friend class ShrineDelegate;
};

class ShrineDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	ShrineDelegate(MapController &, CRewardableObject &);

	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;

private:
	MapController & controller;
	CRewardableObject & shrine;
};
