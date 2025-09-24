#include <qdir.h>
#include "usr_data_mgr.h"

UsrDataMgr::UsrDataMgr(QWidget * parent)
    : QWidget{parent}
{
    return;
}

QString UsrDataMgr::setUsrDataPath()
{
    return QString();
}

QString UsrDataMgr::moveUsrDataDir(QString previousPath, QString currentPath)
{
    //
    // Requested directory can either be read-only or immutable.
    //
    if(QFileInfo(currentPath).exists() && !QFileInfo(currentPath).isWritable())
    {
        return QString();
    }

    QDir().rename(previousPath, currentPath);

    return currentPath;
}
