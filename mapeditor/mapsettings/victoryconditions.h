/*
 * victoryconditions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#ifndef VICTORYCONDITIONS_H
#define VICTORYCONDITIONS_H

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

	void initialize(const CMap & map) override;
	void update(CMap & map) override;

private slots:
	void on_victoryComboBox_currentIndexChanged(int index);

private:
	Ui::VictoryConditions *ui;
	const CMap * mapPointer = nullptr;

	QComboBox * victoryTypeWidget = nullptr;
	QComboBox * victorySelectWidget = nullptr;
	QLineEdit * victoryValueWidget = nullptr;
};

#endif // VICTORYCONDITIONS_H
