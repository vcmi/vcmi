#ifndef REWARDSWIDGET_H
#define REWARDSWIDGET_H

#include <QDialog>

namespace Ui {
class RewardsWidget;
}

class RewardsWidget : public QDialog
{
	Q_OBJECT

public:
	explicit RewardsWidget(QWidget *parent = nullptr);
	~RewardsWidget();

private:
	Ui::RewardsWidget *ui;
};

#endif // REWARDSWIDGET_H
