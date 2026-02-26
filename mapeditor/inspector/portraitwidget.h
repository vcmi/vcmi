/*
 * portraitwidget.h, part of VCMI engine
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

namespace Ui {
class PortraitWidget;
}

class PortraitWidget : public QDialog
{
	Q_OBJECT

public:
	explicit PortraitWidget(CGHeroInstance &, QWidget *parent = nullptr);
	~PortraitWidget();

	void obtainData();
	void commitChanges();
	
	void resizeEvent(QResizeEvent *) override;

private slots:
	void on_isDefault_toggled(bool checked);

	void on_buttonNext_clicked();

	void on_buttonPrev_clicked();

private:
	void drawPortrait();
	
	Ui::PortraitWidget *ui;
	QGraphicsScene scene;
	QPixmap pixmap;

	CGHeroInstance & hero;
	int portraitIndex;
};

class PortraitDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	PortraitDelegate(CGHeroInstance &);

	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const override;

private:
	CGHeroInstance & hero;
};
