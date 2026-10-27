/*
 * questsourcewidget.h, part of VCMI engine
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
#include "lib/mapObjects/Quest.h"


namespace Ui {
class QuestWidget;
}

class MapController;
class TextIdentifier;
class RewardWidget;
class QuestWidget;

class QuestSourceWidget : public QDialog
{
	Q_OBJECT

public:
	explicit QuestSourceWidget(MapController &, QuestSource &, QWidget *parent = nullptr);
	~QuestSourceWidget();
	
	void obtainData();
	void selectQuest(int index);
	void loadQuestData();
	bool commitChanges();

private slots:
	void on_questsList_currentRowChanged(int row);
	void on_addQuestButton_clicked();
	void on_rewardButton_clicked();
	void on_moveUpButton_clicked();
	void on_moveDownButton_clicked();
	void on_deleteQuestButton_clicked();

private:
	void prepareQuestsList(const std::shared_ptr<Quest> & questToSelect = nullptr);
	void setTranslationIdentifiers();
	void setTranslation(MetaString & metastring, const TextIdentifier & identifier, const std::string & translation);
	void shiftSelectedQuest(int shift);
	void disableReward(bool disabled);
	void disableQuest(bool disabled);
	
	QuestSource & questSource;
	std::shared_ptr<Quest> selectedQuest;
	bool questDataLoaded = false;
	MapController & controller;
	Ui::QuestWidget *ui;
	std::unique_ptr<RewardWidget> rewardWidget;
	std::unique_ptr<QuestWidget> questWidget;
};

class QuestSourceDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	QuestSourceDelegate(MapController &, QuestSource &);
	
	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	
protected:
	bool eventFilter(QObject * object, QEvent * event) override;

private:
	QuestSource & questSource;
	MapController & controller;
};
