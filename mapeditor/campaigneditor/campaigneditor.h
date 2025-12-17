/*
 * campaigneditor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>

#include "campaignview.h"

#include "../StdInc.h"
#include "../../lib/constants/EntityIdentifiers.h"

class CampaignState;

namespace Ui {
class CampaignEditor;
}

class CampaignEditor : public QWidget
{
	Q_OBJECT

public:
	explicit CampaignEditor();
	~CampaignEditor();

	void redraw();

	static void showCampaignEditor(QWidget *parent);

private slots:
	void on_actionOpen_triggered();
	void on_actionOpenSet_triggered();
	void on_actionSave_as_triggered();
	void on_actionNew_triggered();
	void on_actionSave_triggered();
	void on_actionCampaignProperties_triggered();
	void on_actionScenarioProperties_triggered();
	
private:
	bool getAnswerAboutUnsavedChanges();
	void setTitle();
	void changed();
	bool validate();
	void saveCampaign();

	void closeEvent(QCloseEvent *event) override;

	Ui::CampaignEditor *ui;

	std::unique_ptr<CampaignScene> campaignScene;

	QString filename;
	bool unsaved = false;
	CampaignScenarioID selectedScenario;
	std::shared_ptr<CampaignState> campaignState;
};
