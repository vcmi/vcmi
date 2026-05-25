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

VCMI_LIB_NAMESPACE_BEGIN
class CampaignState;
class CMap;
class EditorCallback;
VCMI_LIB_NAMESPACE_END

VCMI_LIB_USING_NAMESPACE

namespace Ui {
class CampaignEditor;
}

class CampaignEditor : public QWidget
{
	Q_OBJECT

public:
	explicit CampaignEditor(EditorCallback * cb);
	~CampaignEditor();

	void redraw();

	static void showCampaignEditor(QWidget *parent, EditorCallback * cb);
	static void showCampaignEditor(QWidget *parent, const QString &campaignFile, EditorCallback * cb);
	static std::unique_ptr<CMap> tryToOpenMap(QWidget* parent, std::shared_ptr<CampaignState> state, CampaignScenarioID scenario, EditorCallback * cb);

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
	void loadCampaignFile(const QString & filenameSelect);

	void closeEvent(QCloseEvent *event) override;
	void changeEvent(QEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;

	Ui::CampaignEditor *ui;

	std::unique_ptr<CampaignScene> campaignScene;

	QString filename;
	bool unsaved = false;
	CampaignScenarioID selectedScenario;
	std::shared_ptr<CampaignState> campaignState;
	EditorCallback * cb;
};
