/*
 * helper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "helper.h"
#include "mapcontroller.h"

#include <QApplication>
#include <QScreen>
#include <QToolButton>
#include <QResizeEvent>
#include <QDialog>
#include <QPainterPath>

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CMemoryBuffer.h"
#include "../lib/filesystem/CFilesystemLoader.h"
#include "../lib/filesystem/CZipSaver.h"
#include "../lib/campaign/CampaignHandler.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/MapFormatJson.h"
#include "../lib/mapping/MapFormat.h"
#include "../lib/modding/ModIncompatibility.h"
#include "../lib/rmg/CRmgTemplate.h"
#include "../lib/serializer/JsonSerializer.h"
#include "../lib/serializer/JsonDeserializer.h"
#include "../lib/serializer/CSaveFile.h"

CloseButtonPositioner::CloseButtonPositioner(QDialog * parent, QToolButton * btn)
	: QObject(parent), dialog(parent), closeButton(btn)
{}

void CloseButtonPositioner::reposition()
{
	if(!dialog || !closeButton)
		return;

	QPainterPath path;
	path.addRoundedRect(QRectF(dialog->rect()), 6.0, 6.0);
	dialog->setMask(QRegion(path.toFillPolygon().toPolygon()));

	closeButton->move(dialog->width() - closeButton->width() - 4, 4);
}

bool CloseButtonPositioner::eventFilter(QObject * obj, QEvent * ev)
{
	switch(ev->type())
	{
		case QEvent::Show:
		case QEvent::Resize:
		case QEvent::WindowActivate:
			reposition();
			closeButton->show();
			closeButton->raise();
			break;
		case QEvent::Hide:
		case QEvent::Close:
			closeButton->hide();
			break;
		default:
			break;
	}
	return false;
}

void Helper::decorateDialog(QDialog * dialog)
{
#ifdef VCMI_MOBILE
	dialog->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
	dialog->setStyleSheet("QDialog { border: 2px solid rgba(0,0,0,160); border-radius: 6px; }");

	auto * closeButton = new QToolButton(dialog);
	closeButton->setObjectName("closeButton");
	closeButton->setText(QStringLiteral("X"));
	closeButton->setFixedSize(22, 22);
	closeButton->setMask(QRegion(0, 0, 22, 22, QRegion::Ellipse));
	closeButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
	closeButton->setCursor(Qt::PointingHandCursor);
	QFont closeFont = closeButton->font();
	closeFont.setBold(true);
	closeButton->setFont(closeFont);
	closeButton->setStyleSheet(
		"QToolButton#closeButton { border: none; border-radius: 11px; padding: 1px; background: #c0392b; color: white; font-weight: 700; }"
		"QToolButton#closeButton:hover { background: #e74c3c; }"
		"QToolButton#closeButton:pressed { background: #922b21; }");
	closeButton->setToolTip(QObject::tr("Close"));
	QObject::connect(closeButton, &QToolButton::clicked, dialog, &QDialog::close);
	QObject::connect(dialog, &QDialog::destroyed, closeButton, &QToolButton::close);

	dialog->installEventFilter(new CloseButtonPositioner(dialog, closeButton));

	// Center on screen
	const QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
	dialog->move(screenGeometry.center() - dialog->rect().center());
#else
	dialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
#endif
}

ResourcePath addFilesystemAndGetResource(const QString & filenameSelect, EResType type, const std::string & typeName)
{
	QFileInfo fi(filenameSelect);
	std::string fname = fi.fileName().toStdString();
	std::string fdir = fi.dir().path().toStdString();
	
	ResourcePath resId("MAPEDITOR/" + fname, type);
	
	//addFilesystem takes care about memory deallocation if case of failure, no memory leak here
	auto mapEditorFilesystem = std::make_unique<CFilesystemLoader>("MAPEDITOR/", fdir, 0);
	CResourceHandler::removeFilesystem("local", "mapEditor");
	CResourceHandler::addFilesystem("local", "mapEditor", std::move(mapEditorFilesystem));
	
	if(!CResourceHandler::get("mapEditor")->existsResource(resId))
		throw std::runtime_error("Cannot open " + typeName + " from this folder");

	return resId;
}

std::unique_ptr<CMap> Helper::openMapInternal(const QString & filenameSelect, IGameInfoCallback * cb)
{
	auto resId = addFilesystemAndGetResource(filenameSelect, EResType::MAP, "map");
	
	CMapService mapService;
	if(auto header = mapService.loadMapHeader(resId, true))
	{
		auto missingMods = CMapService::verifyMapHeaderMods(*header);
		ModIncompatibility::ModList modList;
		for(const auto & m : missingMods)
			modList.push_back(m.second.name);
		
		if(!modList.empty())
			throw ModIncompatibility(modList);
		
		return mapService.loadMap(resId, cb);
	}
	else
		throw std::runtime_error("Corrupted map");
}

std::shared_ptr<CampaignState> Helper::openCampaignInternal(const QString & filenameSelect)
{
	auto resId = addFilesystemAndGetResource(filenameSelect, EResType::CAMPAIGN, "campaign");

	if(auto campaign = CampaignHandler::getCampaign(resId.getName()))
		return campaign;
	else
		throw std::runtime_error("Corrupted campaign");
}

std::map<std::string, std::shared_ptr<CRmgTemplate>> Helper::openTemplateInternal(const QString & filenameSelect)
{
	auto resId = addFilesystemAndGetResource(filenameSelect, EResType::JSON, "template");

	auto data = CResourceHandler::get()->load(resId)->readAll();
	JsonNode nodes(reinterpret_cast<std::byte *>(data.first.get()), data.second, resId.getName());

	nodes.setModScope(ModScope::scopeGame());

	std::map<std::string, std::shared_ptr<CRmgTemplate>> templates;
	for(auto & node : nodes.Struct())
	{
		JsonDeserializer handler(nullptr, node.second);
		auto rmg = std::make_shared<CRmgTemplate>();
		rmg->serializeJson(handler);
		rmg->validate();
		templates[node.first] = rmg;
	}
	
	return templates;
}

void Helper::saveCampaign(std::shared_ptr<CampaignState> campaignState, const QString & filename)
{
	auto jsonCampaign = CampaignHandler::writeHeaderToJson(*campaignState);
	
	auto io = std::make_shared<CDefaultIOApi>();
	auto saver = std::make_shared<CZipSaver>(io, filename.toStdString());
	for(auto & scenario : campaignState->allScenarios())
	{
		EditorCallback cb(nullptr);
		auto map = campaignState->getMap(scenario, &cb);
		cb.setMap(map.get());
		MapController::repairMap(map.get());
		CMemoryBuffer serializeBuffer;
		{
			CMapSaverJson jsonSaver(&serializeBuffer);
			jsonSaver.saveMap(map);
		}

		auto mapName = boost::algorithm::to_lower_copy(campaignState->scenario(scenario).mapName);
		if(!boost::ends_with(mapName, ".vmap"))
			mapName = boost::replace_all_copy(mapName, ".h3m", std::string("")) + ".vmap";

		auto stream = saver->addFile(mapName);
		stream->write(reinterpret_cast<const ui8 *>(serializeBuffer.getBuffer().data()), serializeBuffer.getSize());

		jsonCampaign["scenarios"].Vector().push_back(CampaignHandler::writeScenarioToJson(campaignState->scenario(scenario)));
		jsonCampaign["scenarios"].Vector().back()["map"].String() = mapName;
	}

	auto jsonCampaignStr = jsonCampaign.toString();
	saver->addFile("header.json")->write(reinterpret_cast<const ui8 *>(jsonCampaignStr.data()), jsonCampaignStr.length());
}

void Helper::saveTemplate(std::map<std::string, std::shared_ptr<CRmgTemplate>> tpl, const QString & filename)
{
	JsonMap data;

	for(auto & node : tpl)
	{
		JsonNode actual;
		{
			JsonSerializer handler(nullptr, actual);
			node.second->serializeJson(handler);
		}
		data[node.first] = actual;
	}
	
	auto byteData = JsonNode(data).toBytes();
	QByteArray byteDataArray(reinterpret_cast<const char*>(byteData.data()), static_cast<int>(byteData.size()));
	QFile file(filename);

	if(file.open(QIODevice::WriteOnly))
		file.write(byteDataArray);
}
