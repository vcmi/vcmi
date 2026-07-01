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

#include "../lib/filesystem/CCompressedStream.h"
#include "../lib/filesystem/CMemoryStream.h"

#include "../lib/VCMIDirs.h"
#include "helper.h"

constexpr auto DEMO_URL = "http://updates.lokigames.com/loki_demos/heroes3-demo.run";
constexpr auto DEMO_URL_ALTERNATIVE = "https://web.archive.org/web/20150506062114if_/http://updates.lokigames.com/loki_demos/heroes3-demo.run"; // alternative if fails or HTTPS is required

DemoInstaller::DemoInstaller(IDemoInstallerCallback * callback) :
    callback(callback)
{}

void DemoInstaller::downloadProgress(qint64 current, qint64 max)
{
    if(callback)
        callback->onInstallProgress(static_cast<float>(current) / static_cast<float>(max));
}

void DemoInstaller::downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors)
{
	if(failedFiles.empty())
	{
        install(savedFiles.first());
	}
    else if(!usedAlternative)
    {
        logGlobal->warn("Primary demo URL failed, trying alternative: %s", errors.first().toStdString());
        usedAlternative = true;
        dlManager->deleteLater();
        dlManager = nullptr;
        if(callback)
            callback->onInstallProgress(0.0f);
        startDownload(QUrl(DEMO_URL_ALTERNATIVE));
        return;
    }
    else
    {
        logGlobal->error("Download failed: %s", errors.first().toStdString());
        if(callback)
            callback->onInstallError();
    }

	dlManager->deleteLater();
	dlManager = nullptr;
}

void DemoInstaller::download()
{
    startDownload(QUrl(DEMO_URL));
}

void DemoInstaller::startDownload(const QUrl & url)
{
    dlManager = new CDownloadManager();
    
    connect(dlManager, &CDownloadManager::downloadProgress,
        this, [this](const QString &, qint64 current, qint64 max) {
            downloadProgress(current, max);
        });

    connect(dlManager, SIGNAL(finished(QStringList, QStringList, QStringList)),
        this, SLOT(downloadFinished(QStringList, QStringList, QStringList)));

    dlManager->downloadFile(url, "h3demo.run");
}

static QByteArray readStreamFully(CCompressedStream & stream)
{
    QByteArray output;
    std::array<ui8, 32 * 1024> buf;
    while(true)
    {
        si64 before = stream.tell();
        stream.read(buf.data(), static_cast<si64>(buf.size()));
        si64 actual = stream.tell() - before;
        if(actual == 0)
            break;
        output.append(reinterpret_cast<const char *>(buf.data()), static_cast<int>(actual));
    }
    return output;
}

void DemoInstaller::install(QString filename)
{
    QString realFilename = Helper::getRealPath(filename);

    QFile file(realFilename);
    if(!file.open(QIODevice::ReadOnly))
        return;
    QByteArray data = file.readAll();

    QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
    hash.addData(data);
    auto hashStr = hash.result().toHex().toLower();

    if(hashStr != "74c28240794d0aa2fb52cadcd088ab6dd47478c1")
    {
        logGlobal->error("Invalid hash of demo");
        return;
    }

    struct FileEntry
    {
        qint64 offset;
        qint64 size;
        QString name;
    };
    static const std::vector<FileEntry> filesToExtract = {
        { 3817472,   10034,    "Maps/h3demo.h3m"          },
        { 3829248,   625761,   "Video/pgtrnrgh.mjpg"      },
        { 4455936,   305573,   "Video/lbstart.mjpg"       },
        { 4762112,   81673,    "Video/lbloop.mjpg"        },
        { 4844544,   622992,   "Video/pgtrnlft.mjpg"      },
        { 5468160,   2998902,  "Video/nwclogo.mpg"        },
        { 8467968,   459783,   "Video/tavern.mjpg"        },
        { 8928768,   471956,   "Video/win3.mjpg"          },
        { 9713664,   1647741,  "Video/3dologo.mpg"        },
        { 11362304,  10502582, "Data/heroes3.snd"         },
        { 21865472,  22400343, "Data/h3bitmap.lod"        },
        { 44266496,  46633170, "Data/h3sprite.lod"        },
        { 94373888,  560972,   "Mp3/win scenario.mp3"     },
        { 94935552,  55371,    "Mp3/losecombat.mp3"       },
        { 94991872,  1085643,  "Mp3/stronghold.mp3"       },
        { 96078336,  359571,   "Mp3/aitheme0.mp3"         },
        { 96438784,  889055,   "Mp3/water.mp3"            },
        { 97328640,  37665,    "Mp3/losecastle.mp3"       },
        { 97367040,  40196,    "Mp3/surrender battle.mp3" },
        { 97408000,  119129,   "Mp3/ultimatelose.mp3"     },
        { 97527808,  74477,    "Mp3/defend castle.mp3"    },
        { 97603072,  42747,    "Mp3/win battle.mp3"       },
        { 97646592,  1320703,  "Mp3/combat01.mp3"         },
        { 98968064,  577794,   "Mp3/mainmenu.mp3"         },
        { 99546624,  992042,   "Mp3/dirt.mp3"             },
        { 100539392, 50942,    "Mp3/retreat battle.mp3"   },
    };

    // Create a streaming decompressor pointing directly into data (no copy of the raw file).
    // File offsets are ascending, so we only seek forward - no re-decompression needed.
    auto memStream = std::make_unique<CMemoryStream>(
        reinterpret_cast<const ui8 *>(data.constData()) + 5892,
        static_cast<si64>(data.size() - 5892));
    CCompressedStream outerStream(std::move(memStream), true);

    for(const auto & fileEntry : filesToExtract)
    {
        outerStream.seek(fileEntry.offset);

        QByteArray compressed(static_cast<int>(fileEntry.size), Qt::Uninitialized);
        outerStream.read(reinterpret_cast<ui8 *>(compressed.data()), fileEntry.size);

        auto innerMem = std::make_unique<CMemoryStream>(
            reinterpret_cast<const ui8 *>(compressed.constData()),
            static_cast<si64>(compressed.size()));
        CCompressedStream innerStream(std::move(innerMem), true);
        QByteArray fileData = readStreamFully(innerStream);

        QDir dir(QString::fromStdString(VCMIDirs::get().userDataPath().string()));
        QString folder = fileEntry.name.split("/")[0];
        if(!dir.exists(folder))
            dir.mkpath(folder);

        QFile f(dir.filePath(fileEntry.name));
        if (f.open(QIODevice::WriteOnly))
        {
            f.write(fileData);
            f.close();
        }
    }

    if(callback)
        callback->onInstallFinished();
}
