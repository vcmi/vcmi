/*
 * victoryconditions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "abstractsettings.h"

namespace Ui {
class VictoryConditions;
}

class VictoryConditions : public AbstractSettings
{
	Q_OBJECT

public:
	explicit VictoryConditions(QWidget *parent = nullptr);
	~VictoryConditions();

	void initialize(MapController & map) override;
	void update() override;
	
public slots:
	void onObjectSelect();
	void onObjectPicked(const CGObjectInstance *);

private slots:
	void on_victoryComboBox_currentIndexChanged(int index);

private:
	Ui::VictoryConditions *ui;

	QComboBox * victoryTypeWidget = nullptr;
	QComboBox * victorySelectWidget = nullptr;
	QLineEdit * victoryValueWidget = nullptr;
	QToolButton * pickObjectButton = nullptr;
};
