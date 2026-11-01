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
#include "../../lib/VCMIDirs.h"

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
	if(checker->getState() != EUpdateCheckState::UPDATE_AVAILABLE)
		return;

	const UpdateInfo & update = checker->getUpdate();

	ui->labelTitle->setText(tr("VCMI %1 is available. You have %2.").arg(update.version, GameConstants::VCMI_VERSION));

	QString instructions;
	QString actionText;
	actionUrl = update.url;
	actionFallbackUrl = QUrl();

	switch(update.action)
	{
		case EUpdateAction::INSTALL:
			instructions = tr("Your browser will download the installer. Run it once the download finishes: it will close VCMI and update it.");
			actionText = tr("Download Update");
			break;
		case EUpdateAction::DOWNLOAD:
#if defined(VCMI_WINDOWS)
			instructions = tr("Your browser will download the update as a ZIP archive. Close VCMI, then extract the archive into your VCMI folder, replacing existing files.");
#elif defined(VCMI_ANDROID)
			instructions = tr("Your browser will download the update as an APK file. Open it to install the update over the current version. Android may ask you to allow your browser to install apps.");
#elif defined(VCMI_MAC)
			instructions = tr("Your browser will download a disk image. Close VCMI, open it and replace your current VCMI app with the one inside.");
#else
			instructions = tr("Your browser will download the update as an AppImage. Allow it to run as a program, for example with 'chmod +x', and start it.");
#endif
			actionText = tr("Download Update");
			break;
		case EUpdateAction::EXTERNAL:
		{
			actionText = tr("Open Store");
			switch(UpdateChecker::getDistribution())
			{
				case EUpdateDistribution::GOOGLE_PLAY:
					instructions = tr("Update VCMI in Google Play. Google Play releases updates gradually, so the new version may take a few days to appear there.");
					actionUrl = QUrl(QString::fromStdString("market://details?id=" + VCMIDirs::appIdentifier()));
					actionFallbackUrl = QUrl(QString::fromStdString("https://play.google.com/store/apps/details?id=" + VCMIDirs::appIdentifier()));
					break;
				case EUpdateDistribution::FDROID:
					instructions = tr("Update VCMI in your F-Droid app. F-Droid builds new versions itself, so the update may take a few days to appear there.");
					actionUrl = QUrl(QString::fromStdString("https://f-droid.org/packages/" + VCMIDirs::appIdentifier() + "/"));
					break;
				case EUpdateDistribution::TESTFLIGHT:
					instructions = tr("Update VCMI in the TestFlight app. New versions may take a few days to appear there.");
					actionUrl = QUrl("itms-beta://");
					actionFallbackUrl = QUrl("https://testflight.apple.com/join/pJWHSbmu");
					break;
				case EUpdateDistribution::FLATPAK:
					instructions = tr("Update VCMI in your software center or with 'flatpak update eu.vcmi.VCMI'. New versions may take a few days to appear on Flathub.");
					actionUrl = QUrl("https://flathub.org/apps/eu.vcmi.VCMI");
					break;
				default:
#if defined(VCMI_XDG) && !defined(VCMI_ANDROID)
					instructions = tr("Update VCMI with the package manager of your distribution. Some distributions take a long time to ship new versions.");
#else
					instructions = tr("Update VCMI the same way you installed it.");
#endif
					actionText = tr("Open Download Page");
					actionUrl = QUrl("https://vcmi.eu/download/");
					break;
			}
			break;
		}
	}

	instructions += " " + tr("Your saves, settings and mods are kept.");

	ui->buttonAction->setText(actionText);

	ui->labelInstructions->setText(instructions);
	ui->buttonAction->setVisible(actionUrl.isValid());
	ui->labelNotes->setText("• " + update.notes.join("\n• "));
	ui->labelNotes->setVisible(!update.notes.isEmpty());

	ui->buttonChangelog->setVisible(update.changelogUrl.isValid());
	ui->textBrowserChangelog->document()->setMarkdown(update.changelog, QTextDocument::MarkdownFeature::MarkdownNoHTML);
	ui->textBrowserChangelog->setVisible(!update.changelog.isEmpty());
}

void UpdateView::on_buttonAction_clicked()
{
	if(!QDesktopServices::openUrl(actionUrl) && actionFallbackUrl.isValid())
		QDesktopServices::openUrl(actionFallbackUrl);
}

void UpdateView::on_buttonBack_clicked()
{
	Helper::getMainWindow()->switchToStartTab();
}

void UpdateView::on_buttonChangelog_clicked()
{
	QDesktopServices::openUrl(checker->getUpdate().changelogUrl);
}
