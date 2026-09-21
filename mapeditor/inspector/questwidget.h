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
#include "baseinspectoritemdelegate.h"
#include "../../lib/mapObjects/Quest.h"


namespace Ui {
class QuestWidget;
}

class MapController;
class TextIdentifier;

class QuestWidget : public QDialog
{
	Q_OBJECT

public:
	explicit QuestWidget(MapController &, QuestSource &, QWidget *parent = nullptr);
	~QuestWidget();
	
	void obtainData();
	void prepareQuestsList(const std::shared_ptr<Quest> & questToSelect = nullptr);
	void selectQuest(int index);
	void loadQuestData();
	bool commitChanges();

private slots:
	void onTargetPicked(const CGObjectInstance *);
	void on_questsList_currentRowChanged(int row);
	void on_lKillTargetSelect_clicked();
	void on_lCreatureAdd_clicked();
	void on_lCreatureRemove_clicked();
	void on_addQuestButton_clicked();
	void on_deleteQuestButton_clicked();
	void on_deadlineCheckbox_stateChanged(int state);
	void on_repetableCheckbox_stateChanged(int state);

private:
	void onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget);
	void setTranslationIdentifiers();
	void setTranslation(MetaString & metastring, const TextIdentifier & identifier, const std::string & translation);
	void highlightModifiedTabs();
	
	QuestSource & questSource;
	std::shared_ptr<Quest> selectedQuest;
	bool questDataLoaded = false;
	MapController & controller;
	Ui::QuestWidget *ui;
};

class QuestDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	QuestDelegate(MapController &, QuestSource &);
	
	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;
	
protected:
	bool eventFilter(QObject * object, QEvent * event) override;

private:
	QuestSource & questSource;
	MapController & controller;
};
