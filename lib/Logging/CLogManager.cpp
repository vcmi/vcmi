#include "StdInc.h"
#include "CLogManager.h"

#include "CLogger.h"

CLogManager * CLogManager::instance = nullptr;

boost::mutex CLogManager::smx;

CLogManager * CLogManager::get()
{
    TLockGuard _(smx);
    if(!instance)
    {
        instance = new CLogManager();
    }
    return instance;
}

CLogManager::CLogManager()
{

}

CLogManager::~CLogManager()
{
    BOOST_FOREACH(auto & i, loggers)
    {
        delete i.second;
    }
}

void CLogManager::addLogger(CGLogger * logger)
{
    TWriteLock _(mx);
    loggers[logger->getDomain().getName()] = logger;
}

CGLogger * CLogManager::getLogger(const CLoggerDomain & domain)
{
    TReadLock _(mx);
    auto it = loggers.find(domain.getName());
    if(it != loggers.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}
