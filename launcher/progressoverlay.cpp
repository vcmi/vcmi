/*
 * progressoverlay.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "progressoverlay.h"
#include "../helper.h"
#include <QVBoxLayout>
#include <QPalette>

ProgressOverlay::ProgressOverlay(QWidget *parent, int topOffsetPx)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, parent->palette().color(QPalette::Window));
    setPalette(pal);

    setGeometry(parent->rect().adjusted(0, topOffsetPx, 0, 0));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 600;");

    fileLabel = new QLabel(this);
    fileLabel->setAlignment(Qt::AlignCenter);
    fileLabel->setWordWrap(true);

    progressBar = new QProgressBar(this);
    progressBar->setMinimumHeight(18);
    setIndeterminate(true);

    layout->addStretch();
    layout->addWidget(titleLabel);
    layout->addWidget(fileLabel);
    layout->addWidget(progressBar);
    layout->addStretch();

    Helper::keepScreenOn(true);
}

ProgressOverlay::~ProgressOverlay()
{
    Helper::keepScreenOn(false);
}

void ProgressOverlay::setTitle(const QString &title)
{
	titleLabel->setText(title);
}

void ProgressOverlay::setFileName(const QString &name)
{
	fileLabel->setText(name);
}

void ProgressOverlay::setIndeterminate(bool on)
{
	progressBar->setRange(0, on ? 0 : 100);
}

void ProgressOverlay::setRange(int max)
{
	progressBar->setRange(0, qMax(1, max));
	progressBar->setValue(0);
}

void ProgressOverlay::setValue(int val)
{
	progressBar->setValue(val);
}