/*
 * QuestGuardWidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../helper.h"
#include "questguardwidget.h"
#include "ui_questguardwidget.h"
#include "segments/questwidget.h"

#include "../mapcontroller.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/constants/StringConstants.h"

#include <translator.h>

QuestGuardWidget::QuestGuardWidget(MapController & controller, QuestSource & questGuard, QWidget *parent) :
	QDialog(parent),
	controller(controller),
	questGuard(questGuard),
	quest(questGuard.allQuestsEditor().size() ? *questGuard.allQuestsEditor()[0].get() : questGuard.addQuest()),
	ui(new Ui::QuestGuardWidget)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	ui->setupUi(this);
	Helper::decorateDialog(this);
	questWidget = std::make_unique<QuestWidget>(controller,  this);
	questWidget->showMapAction([this](){this->hide();});
	questWidget->returnToWidgetAction([this](){this->show();});
	questWidget->canMakeQuestRepeat(false);
	ui->questContainer->layout()->addWidget(questWidget.get());

	ui->firstVisitText->clear();
	ui->firstVisitText->append(QString::fromStdString(quest.firstVisitText.toString(&Translator::instance())));

	ui->nextVisitText->clear();
	ui->nextVisitText->append(QString::fromStdString(quest.nextVisitText.toString(&Translator::instance())));
}

QuestGuardWidget::~QuestGuardWidget()
{
	delete ui;
}

void QuestGuardWidget::obtainData()
{
	questWidget->loadQuest(&quest);
}

bool QuestGuardWidget::commitChanges()
{
	questWidget->commit();

	setTranslation(quest.firstVisitText, TextIdentifier("quest", questGuard.instanceName, "firstVisit"), ui->firstVisitText->toPlainText().toStdString());
	setTranslation(quest.nextVisitText, TextIdentifier("quest", questGuard.instanceName, "nextVisit"), ui->nextVisitText->toPlainText().toStdString());
	return true;
}

void QuestGuardWidget::setTranslation(MetaString & metastring, const TextIdentifier & identifier, const std::string & translation)
{
	metastring = MetaString::createFromTextID(mapRegisterLocalizedString(
		"map", *controller.map(), identifier, translation));
	if(translation.empty())
		metastring.clear();
}

QuestGuardDelegate::QuestGuardDelegate(MapController & c, QuestSource & t): controller(c), questGuard(t), BaseInspectorItemDelegate()
{
}

QWidget * QuestGuardDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new QuestGuardWidget(controller, questGuard, parent);
}

void QuestGuardDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if(auto *ed = qobject_cast<QuestGuardWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void QuestGuardDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if(auto *ed = qobject_cast<QuestGuardWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

bool QuestGuardDelegate::eventFilter(QObject * object, QEvent * event)
{
	if(auto * ed = qobject_cast<QuestGuardWidget *>(object))
	{
		if(event->type() == QEvent::Hide || event->type() == QEvent::FocusOut)
			return false;
		if(event->type() == QEvent::Close)
		{
			commitData(ed);
			closeEditor(ed);
			return true;
		}
	}
	return QStyledItemDelegate::eventFilter(object, event);
}
