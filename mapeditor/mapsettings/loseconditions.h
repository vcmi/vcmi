#ifndef LOSECONDITIONS_H
#define LOSECONDITIONS_H
/*
 * loseconditions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

	void initialize(const CMap & map) override;
	void update(CMap & map) override;

private slots:
	void on_loseComboBox_currentIndexChanged(int index);

private:
	Ui::LoseConditions *ui;
	const CMap * mapPointer = nullptr;

	QComboBox * loseTypeWidget = nullptr;
	QComboBox * loseSelectWidget = nullptr;
	QLineEdit * loseValueWidget = nullptr;
};

#endif // LOSECONDITIONS_H
