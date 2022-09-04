/*
 * updatedialog_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "updatedialog_moc.h"
#include "ui_updatedialog_moc.h"

#include "../lib/CConfigHandler.h"
#include "../lib/GameConstants.h"

#include <QNetworkReply>
#include <QNetworkRequest>

UpdateDialog::UpdateDialog(bool calledManually, QWidget *parent):
	QDialog(parent),
	ui(new Ui::UpdateDialog),
	calledManually(calledManually)
{
	ui->setupUi(this);
	
	if(calledManually)
	{
		setWindowModality(Qt::ApplicationModal);
		show();
	}
	
	connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
	
	if(settings["launcher"]["updateOnStartup"].Bool())
		ui->checkOnStartup->setCheckState(Qt::CheckState::Checked);
	
	currentVersion = GameConstants::VCMI_VERSION;
	
	setWindowTitle(QString::fromStdString(currentVersion));
	
#ifdef VCMI_WINDOWS
	platformParameter = "windows";
#elif defined(VCMI_MAC)
	platformParameter = "macos";
#elif defined(VCMI_IOS)
	platformParameter = "ios";
#elif defined(VCMI_ANDROID)
	platformParameter = "android";
#elif defined(VCMI_UNIX)
	platformParameter = "linux";
#endif
	
	QString url = QString::fromStdString(settings["launcher"]["updateConfigUrl"].String());
		
	QNetworkReply *response = networkManager.get(QNetworkRequest(QUrl(url)));
	
	connect(response, &QNetworkReply::finished, [&, response]{
		response->deleteLater();
		
		if(response->error() != QNetworkReply::NoError)
		{
			ui->versionLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
			ui->versionLabel->setText("Network error");
			ui->plainTextEdit->setPlainText(response->errorString());
			return;
		}
		
		auto byteArray = response->readAll();
		JsonNode node(byteArray.constData(), byteArray.size());
		loadFromJson(node);
	});
}

UpdateDialog::~UpdateDialog()
{
	delete ui;
}

void UpdateDialog::showUpdateDialog(bool isManually)
{
	UpdateDialog * dialog = new UpdateDialog(isManually);
	
	dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void UpdateDialog::on_checkOnStartup_stateChanged(int state)
{
	Settings node = settings.write["launcher"]["updateOnStartup"];
	node->Bool() = ui->checkOnStartup->isChecked();
}

void UpdateDialog::loadFromJson(const JsonNode & node)
{
	if(node.getType() != JsonNode::JsonType::DATA_STRUCT ||
	   node["updateType"].getType() != JsonNode::JsonType::DATA_STRING ||
	   node["version"].getType() != JsonNode::JsonType::DATA_STRING ||
	   node["changeLog"].getType() != JsonNode::JsonType::DATA_STRING ||
	   node.getType() != JsonNode::JsonType::DATA_STRUCT) //we need at least one link - other are optional
	{
		ui->plainTextEdit->setPlainText("Cannot read JSON from url or incorrect JSON data");
		return;
	}
	
	//check whether update is needed
	bool isFutureVersion = true;
	std::string newVersion = node["version"].String();
	for(auto & prevVersion : node["history"].Vector())
	{
		if(prevVersion.String() == currentVersion)
			isFutureVersion = false;
	}
		
	if(isFutureVersion || currentVersion == newVersion)
	{
		if(!calledManually)
			close();
		
		return;
	}
	
	if(!calledManually)
	{
		setWindowModality(Qt::ApplicationModal);
		show();
	}
	
	const auto updateType = node["updateType"].String();
	
	QString bgColor;
	if(updateType == "minor")
		bgColor = "gray";
	else if(updateType == "major")
		bgColor = "orange";
	else if(updateType == "critical")
		bgColor = "red";
	
	ui->versionLabel->setStyleSheet(QString("QLabel { background-color : %1; color : black; }").arg(bgColor));
	ui->versionLabel->setText(QString::fromStdString(newVersion));
	ui->plainTextEdit->setPlainText(QString::fromStdString(node["changeLog"].String()));
	
	QString downloadLink = QString::fromStdString(node["downloadLinks"]["other"].String());
	if(node["downloadLinks"][platformParameter].getType() == JsonNode::JsonType::DATA_STRING)
		downloadLink = QString::fromStdString(node["downloadLinks"][platformParameter].String());
	
	ui->downloadLink->setText(QString{"<a href=\"%1\">Download page</a>"}.arg(downloadLink));
}
