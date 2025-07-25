/*
 * templateeditor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>

#include "templateview.h"

#include "../StdInc.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/GameConstants.h"

class CRmgTemplate;
class CardItem;
class LineItem;
namespace rmg {
class ZoneOptions;
}

namespace Ui {
class TemplateEditor;
}

class TemplateEditor : public QWidget
{
	Q_OBJECT

public:
	explicit TemplateEditor();
	~TemplateEditor();

	static void showTemplateEditor();

private slots:
	void on_actionOpen_triggered();
	void on_actionSave_as_triggered();
	void on_actionNew_triggered();
	void on_actionSave_triggered();
	void on_actionAutoPosition_triggered();
	void on_actionZoom_in_triggered();
	void on_actionZoom_out_triggered();
	void on_actionZoom_auto_triggered();
	void on_actionZoom_reset_triggered();
	void on_actionAddZone_triggered();
	void on_actionRemoveZone_triggered();
	void on_comboBoxTemplateSelection_activated(int index);
	void on_pushButtonAddSubTemplate_clicked();
	void on_pushButtonRemoveSubTemplate_clicked();
	void on_pushButtonRenameSubTemplate_clicked();
	void on_spinBoxZoneVisPosX_valueChanged();
	void on_spinBoxZoneVisPosY_valueChanged();
	void on_doubleSpinBoxZoneVisSize_valueChanged();
	void on_comboBoxZoneType_currentTextChanged(const QString &text);
	void on_comboBoxZoneOwner_currentTextChanged(const QString &text);
	void on_spinBoxZoneSize_valueChanged();
	void on_spinBoxTownCountPlayer_valueChanged();
	void on_spinBoxCastleCountPlayer_valueChanged();
	void on_spinBoxTownDensityPlayer_valueChanged();
	void on_spinBoxCastleDensityPlayer_valueChanged();
	void on_spinBoxTownCountNeutral_valueChanged();
	void on_spinBoxCastleCountNeutral_valueChanged();
	void on_spinBoxTownDensityNeutral_valueChanged();
	void on_spinBoxCastleDensityNeutral_valueChanged();
	void on_checkBoxMatchTerrainToTown_stateChanged(int state);
	void on_checkBoxTownsAreSameType_stateChanged(int state);
	void on_comboBoxMonsterStrength_currentTextChanged(const QString &text);
	void on_spinBoxZoneId_valueChanged();
	void on_spinBoxZoneLinkTowns_valueChanged();
	void on_spinBoxZoneLinkMines_valueChanged();
	void on_spinBoxZoneLinkTerrain_valueChanged();
	void on_spinBoxZoneLinkTreasure_valueChanged();
	void on_spinBoxZoneLinkCustomObjects_valueChanged();
	void on_checkBoxZoneLinkTowns_stateChanged(int state);
	void on_checkBoxZoneLinkMines_stateChanged(int state);
	void on_checkBoxZoneLinkTerrain_stateChanged(int state);
	void on_checkBoxZoneLinkTreasure_stateChanged(int state);
	void on_checkBoxZoneLinkCustomObjects_stateChanged(int state);
	void on_checkBoxAllowedWaterContentNone_stateChanged(int state);
	void on_checkBoxAllowedWaterContentNormal_stateChanged(int state);
	void on_checkBoxAllowedWaterContentIslands_stateChanged(int state);
	void on_pushButtonConnectionAdd_clicked();
	void on_pushButtonOpenTerrainTypes_clicked();
	void on_pushButtonOpenBannedTerrainTypes_clicked();
	void on_pushButtonAllowedTowns_clicked();
	void on_pushButtonBannedTowns_clicked();
	void on_pushButtonTownHints_clicked();
	void on_pushButtonAllowedMonsters_clicked();
	void on_pushButtonBannedMonsters_clicked();
	void on_pushButtonTreasure_clicked();
	void on_pushButtonMines_clicked();
	void on_pushButtonCustomObjects_clicked();
	void on_pushButtonEntitiesBannedSpells_clicked();
	void on_pushButtonEntitiesBannedArtifacts_clicked();
	void on_pushButtonEntitiesBannedSkills_clicked();
	void on_pushButtonEntitiesBannedHeroes_clicked();
	
private:
	bool getAnswerAboutUnsavedChanges();
	void setTitle();
	void changed();
	void saveTemplate();
	void initContent();
	void loadContent(bool autoPosition = false);
	void saveContent();
	void loadZoneMenuContent(bool onlyPosition = false);
	void saveZoneMenuContent();
	void loadZoneConnectionMenuContent();
	void updateConnectionLines(bool recreate = false);
	void autoPositionZones();
	void updateZonePositions();
	QString getZoneToolTip(std::shared_ptr<rmg::ZoneOptions> zone);
	void updateZoneCards(TRmgTemplateZoneId id = -1);

	void closeEvent(QCloseEvent *event) override;

	Ui::TemplateEditor *ui;

	std::unique_ptr<TemplateScene> templateScene;

	QString filename;
	bool unsaved = false;
	std::map<std::string, std::shared_ptr<CRmgTemplate>> templates;
	std::string selectedTemplate;
	int selectedZone;

	std::map<TRmgTemplateZoneId, CardItem *> cards;
	std::vector<LineItem *> lines;
};
