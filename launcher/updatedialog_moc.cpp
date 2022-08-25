#include "updatedialog_moc.h"
#include "ui_updatedialog_moc.h"

#include "../lib/CConfigHandler.h"
#include "../lib/GameConstants.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

UpdateDialog::UpdateDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::UpdateDialog)
{
	ui->setupUi(this);
	
	connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
	
	if(settings["launcher"]["updateOnStartup"].Bool() == true)
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
#elif define(VCMI_UNIX)
	platformParameter = "linux";
#endif
	
	QString url = QString::fromStdString(settings["launcher"]["updateConfigUrl"].String());
	
	QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
	
	QNetworkReply *response = manager->get(QNetworkRequest(QUrl(url)));
	
	QObject::connect(response, &QNetworkReply::finished, [&, response]{
		response->deleteLater();
		response->manager()->deleteLater();
		if(response->error() != QNetworkReply::NoError)
		{
			ui->plainTextEdit->setPlainText(response->errorString());
			return;
		}
		
		//auto const contentType = response->header(QNetworkRequest::ContentTypeHeader).toString();
		//htmlContent = contentType;
		auto byteArray = response->readAll();
		JsonNode node(byteArray.constData(), byteArray.size());
		loadFromJson(node);
		//QString html;
		//for(int i = 0; i < 200 && response->isReadable(); ++i) //just limit somehow to avoid long queries
			//html += QString::fromUtf8(response->readLine(256));
		
		
	}) && manager.take();
}

UpdateDialog::~UpdateDialog()
{
	delete ui;
}

void UpdateDialog::showUpdateDialog()
{
	UpdateDialog * dialog = new UpdateDialog;
	
	dialog->setWindowModality(Qt::ApplicationModal);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

void UpdateDialog::on_checkOnStartup_stateChanged(int state)
{
	Settings node = settings.write["launcher"]["updateOnStartup"];
	node->Bool() = (state == 2 ? true : false);
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
	std::string newVersion = node["version"].String();
	if(currentVersion == newVersion)
		return;
	
	if(node["updateType"].String() == "minor")
		ui->versionLabel->setStyleSheet("QLabel { background-color : gray; color : black; }");
	if(node["updateType"].String() == "major")
		ui->versionLabel->setStyleSheet("QLabel { background-color : yellow; color : black; }");
	if(node["updateType"].String() == "critical")
		ui->versionLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
	
	ui->versionLabel->setText(QString::fromStdString(newVersion));
	ui->plainTextEdit->setPlainText(QString::fromStdString(node["changeLog"].String()));
	
	QString downloadLink = QString::fromStdString(node["downloadLinks"]["other"].String());
	if(node["downloadLinks"][platformParameter].getType() == JsonNode::JsonType::DATA_STRING)
		downloadLink = QString::fromStdString(node["downloadLinks"][platformParameter].String());
	
	QString downloadHtml("<a href=\"");
	downloadHtml += downloadLink + "\">link</a>";
	
	ui->downloadLink->setText(downloadHtml);
}
