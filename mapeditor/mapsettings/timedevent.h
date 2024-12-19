/*
 * timedevent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QDialog>

#include "mapcontroller.h"

namespace Ui {
class TimedEvent;
}

class TimedEvent : public QDialog
{
	Q_OBJECT

public:
	explicit TimedEvent(MapController & map, QListWidgetItem *, QWidget * parent = nullptr);
	~TimedEvent();

private slots:

	void on_TimedEvent_finished(int result);

	void on_addObjectToDelete_clicked();
	void on_removeObjectToDelete_clicked();
	void onObjectPicked(const CGObjectInstance * obj);
	void insertObjectToDelete(const CGObjectInstance * obj);
	void on_pushButton_clicked();

	void on_resources_itemDoubleClicked(QTableWidgetItem * item);

private:
	MapController & controller;
	Ui::TimedEvent * ui;
	QListWidgetItem * target;
};
