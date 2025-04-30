/*
 * heroartifactswidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QDialog>
#include "baseinspectoritemdelegate.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/entities/artifact/CArtifactFittingSet.h"

namespace Ui {
class HeroArtifactsWidget;
}

class MapController;

class HeroArtifactsWidget : public QDialog
{
	Q_OBJECT

public:
	explicit HeroArtifactsWidget(MapController & controller, CGHeroInstance &, QWidget *parent = nullptr);
	~HeroArtifactsWidget();
	
	void obtainData();
	void commitChanges();

private slots:
	void onSaveArtifact(int32_t artifactIndex, ArtifactPosition slot);

 	void on_addButton_clicked();

	void on_removeButton_clicked();

private:
	enum Column
	{
		SLOT, ARTIFACT
	};
	Ui::HeroArtifactsWidget * ui;
	
	MapController & controller;
	CGHeroInstance & hero;
	CArtifactFittingSet fittingSet;

	void addArtifactToTable(int32_t artifactIndex, ArtifactPosition slot);
	
};

class HeroArtifactsDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;
	
	HeroArtifactsDelegate(MapController & controller, CGHeroInstance &);
	
	QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
	void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;
	
private:
	MapController & controller;
	CGHeroInstance & hero;
};
