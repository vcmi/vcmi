#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include "abstractsettings.h"

namespace Ui {
class GeneralSettings;
}

class GeneralSettings : public AbstractSettings
{
	Q_OBJECT

public:
	explicit GeneralSettings(QWidget *parent = nullptr);
	~GeneralSettings();

	void initialize(const CMap & map) override;
	void update(CMap & map) override;

private slots:
	void on_heroLevelLimitCheck_toggled(bool checked);

private:
	Ui::GeneralSettings *ui;
};

#endif // GENERALSETTINGS_H
