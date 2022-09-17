#ifndef SPOILER_H
#define SPOILER_H

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>

class Spoiler : public QWidget {
	Q_OBJECT

private:
	QGridLayout mainLayout;
	QToolButton toggleButton;
	QFrame headerLine;
	QParallelAnimationGroup toggleAnimation;
	QScrollArea contentArea;
	int animationDuration{300};

public:
	explicit Spoiler(const QString & title = "", const int animationDuration = 300, QWidget *parent = 0);
	void setContentLayout(QLayout & contentLayout);
};

#endif // SPOILER_H
