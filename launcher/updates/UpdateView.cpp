/*
 * UpdateView.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "UpdateView.h"
#include "ui_UpdateView.h"

#include "UpdateChecker.h"
#include "../helper.h"
#include "../mainwindow_moc.h"

#include "../../lib/GameConstants.h"

UpdateView::UpdateView(QWidget * parent)
	: QWidget(parent)
	, ui(std::make_unique<Ui::UpdateView>())
	, checker(Helper::getMainWindow()->getUpdateChecker())
{
	ui->setupUi(this);
	connect(checker, &UpdateChecker::stateChanged, this, &UpdateView::refresh);
	refresh();
}

UpdateView::~UpdateView() = default;

void UpdateView::changeEvent(QEvent * event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		refresh();
	}

	QWidget::changeEvent(event);
}

void UpdateView::refresh()
{
	const EUpdateCheckState state = checker->getState();

	ui->labelCurrentVersion->setText(tr("Installed version: %1").arg(GameConstants::VCMI_VERSION));
	ui->buttonCheck->setVisible(state == EUpdateCheckState::NOT_CHECKED || state == EUpdateCheckState::FAILED || state == EUpdateCheckState::NO_UPDATE);
	ui->labelNotes->hide();
	ui->buttonAction->hide();
	ui->buttonChangelog->hide();
	ui->textBrowserChangelog->hide();

	switch(state)
	{
		case EUpdateCheckState::DISABLED:
			ui->labelStatus->setText(tr("This build of VCMI does not check for updates."));
			break;
		case EUpdateCheckState::NOT_CHECKED:
			ui->labelStatus->clear();
			break;
		case EUpdateCheckState::CHECKING:
			ui->labelStatus->setText(tr("Checking for updates..."));
			break;
		case EUpdateCheckState::FAILED:
			ui->labelStatus->setText(tr("Could not check for updates:") + "\n" + checker->getErrorMessage());
			break;
		case EUpdateCheckState::NO_UPDATE:
			ui->labelStatus->setText(tr("You are using the latest version."));
			break;
		case EUpdateCheckState::UPDATE_AVAILABLE:
			showUpdate();
			break;
	}
}

void UpdateView::showUpdate()
{
	const UpdateInfo & update = checker->getUpdate();

	QString status = tr("VCMI %1 has been released.").arg(update.version);
	actionUrl = update.url;
	actionFallbackUrl = QUrl();

	switch(update.action)
	{
		case EUpdateAction::INSTALL:
			ui->buttonAction->setText(tr("Update to %1").arg(update.version));
			break;
		case EUpdateAction::DOWNLOAD:
			ui->buttonAction->setText(tr("Download %1").arg(update.version));
			break;
		case EUpdateAction::EXTERNAL:
		{
			switch(UpdateChecker::getDistribution())
			{
				case EUpdateDistribution::GOOGLE_PLAY:
					status += " " + tr("It may take a few days to appear in Google Play.");
					ui->buttonAction->setText(tr("Open Google Play"));
					actionUrl = QUrl("market://details?id=is.xyz.vcmi");
					actionFallbackUrl = QUrl("https://play.google.com/store/apps/details?id=is.xyz.vcmi");
					break;
				case EUpdateDistribution::FDROID:
					status += " " + tr("It may take a few days to appear in F-Droid.");
					ui->buttonAction->setText(tr("Open F-Droid"));
					actionUrl = QUrl("https://f-droid.org/packages/is.xyz.vcmi/");
					break;
				case EUpdateDistribution::TESTFLIGHT:
					status += " " + tr("It may take a few days to appear in TestFlight.");
					ui->buttonAction->setText(tr("Open TestFlight"));
					actionUrl = QUrl("https://testflight.apple.com/join/pJWHSbmu");
					break;
				case EUpdateDistribution::FLATPAK:
					status += " " + tr("Update VCMI through Flatpak or your software center. It may take a few days to appear there.");
					break;
				default:
#if defined(Q_OS_LINUX) && !defined(VCMI_ANDROID)
					status += " " + tr("Update VCMI the same way you installed it, for example through the package manager of your distribution.");
#else
					status += " " + tr("Update VCMI the same way you installed it.");
#endif
					break;
			}
			break;
		}
	}

	ui->labelStatus->setText(status);
	ui->buttonAction->setVisible(actionUrl.isValid());

	if(!update.notes.isEmpty())
	{
		ui->labelNotes->setText("• " + update.notes.join("\n• "));
		ui->labelNotes->show();
	}

	ui->buttonChangelog->setVisible(update.changelogUrl.isValid());
	ui->textBrowserChangelog->document()->setMarkdown(update.changelog, QTextDocument::MarkdownFeature::MarkdownNoHTML);
	ui->textBrowserChangelog->setVisible(!update.changelog.isEmpty());
}

void UpdateView::on_buttonAction_clicked()
{
	if(!QDesktopServices::openUrl(actionUrl) && actionFallbackUrl.isValid())
		QDesktopServices::openUrl(actionFallbackUrl);
}

void UpdateView::on_buttonCheck_clicked()
{
	checker->check();
}

void UpdateView::on_buttonChangelog_clicked()
{
	QDesktopServices::openUrl(checker->getUpdate().changelogUrl);
}
