#ifndef USR_DATA_MGR_H
#define USR_DATA_MGR_H

#include <QWidget>

class UsrDataMgr : public QWidget
{
    Q_OBJECT
public:
    explicit UsrDataMgr(QWidget * parent = nullptr);

    /*!
    * \brief setUsrDataPath - set a path to the game data files.
    * \return a path to the game data files.
    * \return empty string if nothing was selected in the directory browser window.
    * \return empty string on directory transfer failure.
    */
    virtual QString setUsrDataPath();
    /*!
    * \brief moveUsrDataDir - move existing game data files from one location to another.
    * \param previousPath - previously set path.
    * \param currentPath - desired path.
    * \return path to the game data directory.
    * \return empty string if parent directory is set as read-only or as immutable.
    */
    virtual QString moveUsrDataDir(QString previousPath, QString currentPath);
signals:
};

#endif // USR_DATA_MGR_H
