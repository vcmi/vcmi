#include "rewardswidget.h"
#include "ui_rewardswidget.h"

RewardsWidget::RewardsWidget(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::RewardsWidget)
{
	ui->setupUi(this);
}

RewardsWidget::~RewardsWidget()
{
	delete ui;
}
