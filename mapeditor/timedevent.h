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
