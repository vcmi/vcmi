/*
 * progressoverlay.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>

class ProgressOverlay final : public QWidget
{
public:
    explicit ProgressOverlay(QWidget *parent, int topOffsetPx = 50);
    ~ProgressOverlay() override;

    void setTitle(const QString &t);
    void setFileName(const QString &name);

    // Switch between indeterminate and determinate progress
    void setIndeterminate(bool on);
    void setRange(int max);
    void setValue(int v);

private:
    QLabel *titleLabel;
    QLabel *fileLabel;
    QProgressBar *progressBar;
};