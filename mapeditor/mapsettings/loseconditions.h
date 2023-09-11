/*
 * loseconditions.h, part of VCMI engine
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
class LoseConditions;
}

class LoseConditions : public AbstractSettings
{
	Q_OBJECT

public:
	explicit LoseConditions(QWidget *parent = nullptr);
	~LoseConditions();

	void initialize(MapController & map) override;
	void update() override;
	
public slots:
	void onObjectSelect();
	void onObjectPicked(const CGObjectInstance *);

private slots:
	void on_loseComboBox_currentIndexChanged(int index);

private:
	Ui::LoseConditions *ui;

	QComboBox * loseTypeWidget = nullptr;
	QComboBox * loseSelectWidget = nullptr;
	QLineEdit * loseValueWidget = nullptr;
	QToolButton * pickObjectButton = nullptr;
};

