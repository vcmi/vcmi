#include "timedevent.h"
#include "ui_timedevent.h"

TimedEvent::TimedEvent(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::TimedEvent)
{
	ui->setupUi(this);
}

TimedEvent::~TimedEvent()
{
	delete ui;
}

void TimedEvent::on_eventResources_clicked()
{

}


void TimedEvent::on_TimedEvent_finished(int result)
{

}

