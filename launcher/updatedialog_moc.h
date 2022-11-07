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

private slots:
    void on_checkOnStartup_stateChanged(int state);

private:
	Ui::UpdateDialog *ui;
	
	std::string currentVersion;
	std::string platformParameter = "other";
	
	QNetworkAccessManager networkManager;
	
	bool calledManually;
	
	void loadFromJson(const JsonNode & node);
};
