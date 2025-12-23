/*
 * portraitwidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "portraitwidget.h"
#include "ui_portraitwidget.h"
#include "../Animation.h"

#include "../lib/GameLibrary.h"
#include "../lib/entities/hero/CHeroHandler.h"

PortraitWidget::PortraitWidget(CGHeroInstance & h, QWidget *parent):
	QDialog(parent),
	ui(new Ui::PortraitWidget),
	hero(h),
	portraitIndex(0)
{
	ui->setupUi(this);
	ui->portraitView->setScene(&scene);
	ui->portraitView->fitInView(scene.itemsBoundingRect(), Qt::KeepAspectRatio);
	show();
}

PortraitWidget::~PortraitWidget()
{
	delete ui;
}

void PortraitWidget::obtainData()
{
	auto portraitSource = hero.getPortraitSource();
	if(portraitSource.isValid())
	{
		portraitIndex = LIBRARY->heroh->getById(portraitSource)->getIndex();
		if(hero.customPortraitSource.isValid())
		{
			ui->isDefault->setChecked(false);
		}
	}
	
	drawPortrait();
}

void PortraitWidget::commitChanges()
{
	if(portraitIndex == LIBRARY->heroh->getById(HeroTypeID(hero.subID))->getIndex())
		hero.customPortraitSource = HeroTypeID::NONE;
	else
		hero.customPortraitSource = LIBRARY->heroh->getByIndex(portraitIndex)->getId();
}

void PortraitWidget::drawPortrait()
{
	static Animation portraitAnimation(AnimationPath::builtin("PortraitsLarge").getOriginalName());
	portraitAnimation.preload();
	auto icon = LIBRARY->heroTypes()->getByIndex(portraitIndex)->getIconIndex();
	pixmap = QPixmap::fromImage(*portraitAnimation.getImage(icon));
	scene.addPixmap(pixmap);
	ui->portraitView->fitInView(scene.itemsBoundingRect(), Qt::KeepAspectRatio);
}

void PortraitWidget::resizeEvent(QResizeEvent *)
{
	ui->portraitView->fitInView(scene.itemsBoundingRect(), Qt::KeepAspectRatio);
}

void PortraitWidget::on_isDefault_toggled(bool checked)
{
	if(checked)
	{
		ui->buttonNext->setEnabled(false);
		ui->buttonPrev->setEnabled(false);
		portraitIndex = LIBRARY->heroh->getById(HeroTypeID(hero.subID))->getIndex();
	}
	else
	{
		ui->buttonNext->setEnabled(true);
		ui->buttonPrev->setEnabled(true);
	}
	drawPortrait();
}


void PortraitWidget::on_buttonNext_clicked()
{
	if(portraitIndex < LIBRARY->heroh->size() - 1)
		++portraitIndex;
	else
		portraitIndex = 0;
	
	drawPortrait();
}


void PortraitWidget::on_buttonPrev_clicked()
{
	if(portraitIndex > 0)
		--portraitIndex;
	else
		portraitIndex = LIBRARY->heroh->size() - 1;
	
	drawPortrait();
}

PortraitDelegate::PortraitDelegate(CGHeroInstance & h)
	: BaseInspectorItemDelegate()
	, hero(h)
{
}

QWidget * PortraitDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new PortraitWidget(hero, parent);
}

void PortraitDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<PortraitWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void PortraitDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<PortraitWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void PortraitDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QMap<int, QVariant> data;
	if(hero.customPortraitSource == HeroTypeID::NONE)
		data[Qt::DisplayRole] = QObject::tr("Default");
	else
		data[Qt::DisplayRole] = QString::fromStdString(hero.customPortraitSource.toHeroType()->getNameTranslated());
	
	model->setItemData(index, data);
}
