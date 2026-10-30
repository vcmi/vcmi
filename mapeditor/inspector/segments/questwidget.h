/*
 * questwidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "StdInc.h"
#include <QWidget>
#include <optional>

class Quest;
class MapController;
class CGObjectInstance;

namespace Ui {
class QuestWidget;
}

class QuestWidget : public QWidget
{
	Q_OBJECT

public:

	explicit QuestWidget(MapController & controller, QWidget *parent);
	~QuestWidget();

	void loadQuest(Quest * reward);
	void commit();
	void clearData();
	bool isQuestLoaded() const;
	void showMapAction(std::optional<std::function<void()>>showMap);
	void returnToWidgetAction(std::optional<std::function<void()>>returnToWidget);
	void canMakeQuestRepeat(bool allow);

private slots:

	void onTargetPicked(const CGObjectInstance *);
	void on_killTargetSelect_clicked();
	void on_creatureAdd_clicked();
	void on_creatureRemove_clicked();
	void on_deadlineCheckbox_stateChanged(int state);
	void on_repeatableCheckbox_clicked();

signals:
	void repeatableClicked();

private:

	void onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget);
	void showData(Quest * reward);
	void printQuestInformation();
	bool targetPickerIsAllowed();

	Ui::QuestWidget *ui;
	Quest * quest = nullptr;
	MapController & controller;
	bool canChangeQuestRepeatability = true;
	std::optional<std::function<void()>> showMap;
	std::optional<std::function<void()>> returnToWidget;
};