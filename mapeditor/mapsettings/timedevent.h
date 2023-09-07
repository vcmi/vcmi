/*
 * timedevent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#ifndef TIMEDEVENT_H
#define TIMEDEVENT_H

#include <QDialog>

namespace Ui {
class TimedEvent;
}

class TimedEvent : public QDialog
{
	Q_OBJECT

public:
	explicit TimedEvent(QWidget *parent = nullptr);
	~TimedEvent();

private slots:
	void on_eventResources_clicked();

	void on_TimedEvent_finished(int result);

private:
	Ui::TimedEvent *ui;
};

#endif // TIMEDEVENT_H
