#include "StdInc.h"
#include "CLogger.h"
#include "LogRecord.h"
#include "ILogTarget.h"
#include "CLogManager.h"

boost::mutex CGLogger::smx;

const std::string CLoggerDomain::DOMAIN_GLOBAL = "global";

CLoggerDomain::CLoggerDomain(const std::string & name) : name(name)
{
    if(name.empty()) throw std::runtime_error("Logger domain cannot be empty.");
}

CLoggerDomain CLoggerDomain::getParent() const
{
    if(isGlobalDomain()) return *this;

    size_t pos = name.find_last_of(".");
    if(pos != std::string::npos)
    {
        return CLoggerDomain(name.substr(0, pos));
    }
    else
    {
        return CLoggerDomain(DOMAIN_GLOBAL);
    }
}

bool CLoggerDomain::isGlobalDomain() const
{
    return name == DOMAIN_GLOBAL;
}

std::string CLoggerDomain::getName() const
{
    return name;
}

CGLogger * CGLogger::getLogger(const CLoggerDomain & domain)
{
    TLockGuard _(smx);
    CGLogger * logger = CLogManager::get()->getLogger(domain);
    if(logger)
    {
        return logger;
    }
    else
    {
        logger = new CGLogger(domain);
        if(domain.isGlobalDomain())
        {
            logger->setLevel(ELogLevel::INFO);
        }
        CLogManager::get()->addLogger(logger);
        return logger;
    }
}

CGLogger * CGLogger::getGlobalLogger()
{
    return getLogger(CLoggerDomain::DOMAIN_GLOBAL);
}

CGLogger::CGLogger(const CLoggerDomain & domain) : domain(domain), level(ELogLevel::INFO)
{
    if(!domain.isGlobalDomain())
    {
        parent = getLogger(domain.getParent());
    }
}

CGLogger::~CGLogger()
{
    BOOST_FOREACH(ILogTarget * target, targets)
    {
        delete target;
    }
}

void CGLogger::trace(const std::string & message) const
{
    log(ELogLevel::TRACE, message);
}

CLoggerStream CGLogger::traceStream() const
{
    return CLoggerStream(*this, ELogLevel::TRACE);
}

void CGLogger::debug(const std::string & message) const
{
    log(ELogLevel::DEBUG, message);
}

CLoggerStream CGLogger::debugStream() const
{
    return CLoggerStream(*this, ELogLevel::DEBUG);
}

void CGLogger::info(const std::string & message) const
{
    log(ELogLevel::INFO, message);
}

CLoggerStream CGLogger::infoStream() const
{
    return CLoggerStream(*this, ELogLevel::INFO);
}

void CGLogger::warn(const std::string & message) const
{
    log(ELogLevel::WARN, message);
}

CLoggerStream CGLogger::warnStream() const
{
    return CLoggerStream(*this, ELogLevel::WARN);
}

void CGLogger::error(const std::string & message) const
{
    log(ELogLevel::ERROR, message);
}

CLoggerStream CGLogger::errorStream() const
{
    return CLoggerStream(*this, ELogLevel::ERROR);
}

void CGLogger::log(ELogLevel::ELogLevel level, const std::string & message) const
{
    if(getEffectiveLevel() <= level)
    {
        callTargets(LogRecord(domain, level, message));
    }
}

ELogLevel::ELogLevel CGLogger::getLevel() const
{
    TReadLock _(mx);
    return level;
}

void CGLogger::setLevel(ELogLevel::ELogLevel level)
{
    TWriteLock _(mx);
    if(domain.isGlobalDomain() && level == ELogLevel::NOT_SET) return;
    this->level = level;
}

const CLoggerDomain & CGLogger::getDomain() const
{
    return domain;
}

void CGLogger::addTarget(ILogTarget * target)
{
    TWriteLock _(mx);
    targets.push_back(target);
}

std::list<ILogTarget *> CGLogger::getTargets() const
{
    TReadLock _(mx);
    return targets;
}

ELogLevel::ELogLevel CGLogger::getEffectiveLevel() const
{
    for(const CGLogger * logger = this; logger != nullptr; logger = logger->parent)
    {
        if(logger->getLevel() != ELogLevel::NOT_SET) return logger->getLevel();
    }

    // This shouldn't be reached, as the root logger must have set a log level
    return ELogLevel::INFO;
}

void CGLogger::callTargets(const LogRecord & record) const
{
    for(const CGLogger * logger = this; logger != nullptr; logger = logger->parent)
    {
        BOOST_FOREACH(ILogTarget * target, logger->getTargets())
        {
            target->write(record);
        }
    }
}

CLoggerStream::CLoggerStream(const CGLogger & logger, ELogLevel::ELogLevel level) : logger(logger), level(level), sbuffer(nullptr)
{

}

CLoggerStream::~CLoggerStream()
{
    if(sbuffer)
    {
        logger.log(level, sbuffer->str());
        delete sbuffer;
        sbuffer = nullptr;
    }
}
