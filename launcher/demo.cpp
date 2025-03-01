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

#include <zlib.h>
#define GZIP_WINDOWS_BIT 15 + 16
#define GZIP_CHUNK_SIZE 32 * 1024

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

bool gzipDecompress(QByteArray input, QByteArray &output)
{
    output.clear();

    if(input.length() > 0)
    {
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;

        int ret = inflateInit2(&strm, GZIP_WINDOWS_BIT);

        if (ret != Z_OK)
            return false;

        char *input_data = input.data();
        int input_data_left = input.length();

        do
        {
            int chunk_size = qMin(GZIP_CHUNK_SIZE, input_data_left);

            if(chunk_size <= 0)
                break;

            strm.next_in = (unsigned char*)input_data;
            strm.avail_in = chunk_size;

            input_data += chunk_size;
            input_data_left -= chunk_size;

            do
            {
                char out[GZIP_CHUNK_SIZE];

                strm.next_out = (unsigned char*)out;
                strm.avail_out = GZIP_CHUNK_SIZE;

                ret = inflate(&strm, Z_NO_FLUSH);

                switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                    break;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                    inflateEnd(&strm);

                    return(false);
                }

                int have = GZIP_CHUNK_SIZE - strm.avail_out;

                if(have > 0)
                    output.append((char*)out, have);

            }
            while(strm.avail_out == 0);

        }
        while(ret != Z_STREAM_END);

        inflateEnd(&strm);

        return ret == Z_STREAM_END;
    }
    else
        return true;
}

void Demo::install(QString filename)
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

    QByteArray compressedData = data.mid(5892);
    QByteArray uncompressedData;
    gzipDecompress(compressedData, uncompressedData);

    struct TarData
    {
        qint64 offset;
        qint64 size;
        QString name;
    };
    std::vector<TarData> filesToExtract = {
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

    for(auto & file : filesToExtract)
    {
        QByteArray tmp = uncompressedData.mid(file.offset, file.size);
        QByteArray tmpUncompressed;
        gzipDecompress(tmp, tmpUncompressed);

        QDir dir(QString::fromStdString(VCMIDirs::get().userDataPath().string()));
        QString folder = file.name.split("/")[0];
        if (!dir.exists(folder))
            dir.mkpath(folder);

        QFile f(dir.filePath(file.name));
        f.open(QIODevice::WriteOnly);
        f.write(tmpUncompressed);
        f.close();
    }

    if(onFinish)
        onFinish();
}