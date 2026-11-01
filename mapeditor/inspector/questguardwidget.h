/*
 * QuestGuardWidget.h, part of VCMI engine
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
#include "baseinspectoritemdelegate.h"
#include "../../lib/mapObjects/Quest.h"


namespace Ui {
class QuestGuardWidget;
}

class MapController;
class QuestWidget;
class TextIdentifier;

class QuestGuardWidget : public QDialog
{
	Q_OBJECT

public:
	explicit QuestGuardWidget(MapController &, QuestSource &, QWidget *parent = nullptr);
	~QuestGuardWidget();
	
	void obtainData();
	bool commitChanges();

private:

	void setTranslation(MetaString & metastring, const TextIdentifier & identifier, const std::string & translation);

	Quest & quest;
	QuestSource & questGuard;
	std::shared_ptr<QuestWidget> questWidget;
	MapController & controller;
	Ui::QuestGuardWidget *ui;
};

class QuestGuardDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	QuestGuardDelegate(MapController &, QuestSource &);
	
	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	
protected:
	bool eventFilter(QObject * object, QEvent * event) override;

private:
	QuestSource & questGuard;
	MapController & controller;
};
