/*
 * ArtifactWidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QDialog>

VCMI_LIB_NAMESPACE_BEGIN
class ArtifactPosition;
class CArtifactFittingSet;
VCMI_LIB_NAMESPACE_END

namespace Ui {
class ArtifactWidget;
}

class ArtifactWidget : public QDialog
{
	Q_OBJECT

public:
	explicit ArtifactWidget(CArtifactFittingSet & fittingSet, QWidget * parent = nullptr);
	~ArtifactWidget();
	
signals:
	void saveArtifact(int32_t artifactIndex, ArtifactPosition slot);
 private slots:
	void fillArtifacts();

private:
	Ui::ArtifactWidget * ui;
	CArtifactFittingSet & fittingSet;
};
