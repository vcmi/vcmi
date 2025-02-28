/*
 * demo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "demo.h"

#include <QByteArray>
#include <QUrl>

#include "modManager/cdownloadmanager_moc.h"

#include "../lib/VCMIDirs.h"
#include "helper.h"

QString DEMO_URL = "http://updates.lokigames.com/loki_demos/heroes3-demo.run";

Demo::Demo(std::function<void ()> onFinish, std::function<void ()> onError, std::function<void (float percent)> onProgress) :
    onFinish(onFinish), onError(onError), onProgress(onProgress)
{}

void Demo::downloadProgress(qint64 current, qint64 max)
{
    if(onProgress)
        onProgress(static_cast<float>(current) / static_cast<float>(max));
}

void Demo::downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors)
{
	if(failedFiles.empty())
        install(savedFiles.first());
    else
    {
        logGlobal->error("Download failed: %s", errors.first().toStdString());
        if(onError)
            onError();
    }

	dlManager->deleteLater();
	dlManager = nullptr;
}

void Demo::download()
{
    QUrl url(DEMO_URL);
    dlManager = new CDownloadManager();
    
    connect(dlManager, SIGNAL(downloadProgress(qint64, qint64)),
        this, SLOT(downloadProgress(qint64, qint64)));

    connect(dlManager, SIGNAL(finished(QStringList, QStringList, QStringList)),
        this, SLOT(downloadFinished(QStringList, QStringList, QStringList)));

    dlManager->downloadFile(url, "h3demo.run");
}

void Demo::install(QString filename)
{
    QString realFilename = Helper::getRealPath(filename);

    QFile file(realFilename);
    if(!file.open(QIODevice::ReadOnly))
        return;
    QByteArray DataFile = file.readAll();

    if(onFinish)
        onFinish();
}