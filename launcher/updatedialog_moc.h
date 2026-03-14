/*
 * updatedialog_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QDialog>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QPixmap>

class QResizeEvent;
class QPaintEvent;
class QShowEvent;

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

VCMI_LIB_NAMESPACE_END

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
	Q_OBJECT

public:
	explicit UpdateDialog(bool calledManually, QWidget *parent = nullptr);
	~UpdateDialog();
	
	static void showUpdateDialog(bool isManually);

	QString releaseUrl;
	QString testingUrl;
	QString releaseVersion;
	QString testingVersion;


protected:
	void resizeEvent(QResizeEvent * event) override;
	void paintEvent(QPaintEvent * event) override;
	void showEvent(QShowEvent * event) override;

private slots:
    void on_checkOnStartup_stateChanged(int state);
    void on_testingBuilds_stateChanged(int state);
    void on_testingBuilds_clicked(bool checked);

	void on_buildChannel_currentIndexChanged(int index);
	void on_tabWidget_currentChanged(int index);

	void on_installButton_clicked();
	void on_closeButton_clicked();
	void on_infoButton_clicked();

private:
	struct TestingBuildState
	{
		QString channel;
		QString version;
		QString commit;
		QString buildDate;
		QString downloadUrl;
		QString changelog;
		bool valid = false;
	};

	Ui::UpdateDialog *ui;
	
	std::string currentVersion;
	std::string currentCommit;
	std::string currentBranch;
	
	QNetworkAccessManager networkManager;
	TestingBuildState betaState;
	TestingBuildState developState;
	QString selectedTestingCommit;
	QString selectedTestingBuildDate;
	QString selectedTestingChannel;
	QString releaseBuildDate;
	bool releaseOffer = false;
	bool testingOffer = false;
	bool testingChannelAutoSelectPending = true;

	bool calledManually;
	QPixmap mobileBackdrop;
	bool mobileBackdropCaptureScheduled = false;

	void loadFromJson(const JsonNode & node, bool testing = false, const QString &channel = QString());
	void fetchChannel(const QString& channel);
	void refreshTestingBuildFromNewest();
	void applySelectedTestingChannel();
	void updateAvailabilityNotice();
	void startDownloadToCacheAndRun(const QUrl& url, const QString& target = QString(), bool targetIsFile = false);
	void updateMobileBackdrop();
	void ensureMobileBackdropCaptured();
	void updateMobileHostGeometry();
};
