#include "StdInc.h"
#include "CLogger.h"

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

boost::recursive_mutex CGLogger::smx;

CGLogger * logGlobal = CGLogger::getGlobalLogger();

CGLogger * logBonus = CGLogger::getLogger(CLoggerDomain("bonus"));

CGLogger * logNetwork = CGLogger::getLogger(CLoggerDomain("network"));

CGLogger * logAi = CGLogger::getLogger(CLoggerDomain("ai"));

CGLogger * CGLogger::getLogger(const CLoggerDomain & domain)
{
    boost::lock_guard<boost::recursive_mutex> _(smx);

    CGLogger * logger = CLogManager::get().getLogger(domain);
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
        CLogManager::get().addLogger(logger);
        return logger;
    }
}

CGLogger * CGLogger::getGlobalLogger()
{
    return getLogger(CLoggerDomain(CLoggerDomain::DOMAIN_GLOBAL));
}

CGLogger::CGLogger(const CLoggerDomain & domain) : domain(domain)
{
    if(domain.isGlobalDomain())
    {
        level = ELogLevel::INFO;
        parent = nullptr;
    }
    else
    {
        level = ELogLevel::NOT_SET;
        parent = getLogger(domain.getParent());
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
    TLockGuard _(mx);
    return level;
}

void CGLogger::setLevel(ELogLevel::ELogLevel level)
{
    TLockGuard _(mx);
    if(domain.isGlobalDomain() && level == ELogLevel::NOT_SET) return;
    this->level = level;
}

const CLoggerDomain & CGLogger::getDomain() const
{
    return domain;
}

void CGLogger::addTarget(unique_ptr<ILogTarget> && target)
{
    TLockGuard _(mx);
    targets.push_back(std::move(target));
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
    TLockGuard _(mx);
    for(const CGLogger * logger = this; logger != nullptr; logger = logger->parent)
    {
        BOOST_FOREACH(auto & target, logger->targets)
        {
            target->write(record);
        }
    }
}

void CGLogger::clearTargets()
{
    TLockGuard _(mx);
    targets.clear();
}

bool CGLogger::isDebugEnabled() const
{
    return getEffectiveLevel() <= ELogLevel::DEBUG;
}

bool CGLogger::isTraceEnabled() const
{
    return getEffectiveLevel() <= ELogLevel::TRACE;
}

boost::recursive_mutex CLogManager::smx;

CLogManager & CLogManager::get()
{
    TLockGuardRec _(smx);
    static CLogManager instance;
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
    TLockGuard _(mx);
    loggers[logger->getDomain().getName()] = logger;
}

CGLogger * CLogManager::getLogger(const CLoggerDomain & domain)
{
    TLockGuard _(mx);
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

CLogFormatter::CLogFormatter() : pattern("%m")
{

}

CLogFormatter::CLogFormatter(const std::string & pattern) : pattern(pattern)
{

}

std::string CLogFormatter::format(const LogRecord & record) const
{
    std::string message = pattern;

    // Format date
    std::stringstream dateStream;
    boost::posix_time::time_facet * facet = new boost::posix_time::time_facet("%H:%M:%S");
    dateStream.imbue(std::locale(dateStream.getloc(), facet));
    dateStream << record.timeStamp;
    boost::algorithm::replace_all(message, "%d", dateStream.str());

    // Format log level
    std::string level;
    switch(record.level)
    {
    case ELogLevel::TRACE:
        level = "TRACE";
        break;
    case ELogLevel::DEBUG:
        level = "DEBUG";
        break;
    case ELogLevel::INFO:
        level = "INFO";
        break;
    case ELogLevel::WARN:
        level = "WARN";
        break;
    case ELogLevel::ERROR:
        level = "ERROR";
        break;
    }
    boost::algorithm::replace_all(message, "%l", level);

    // Format name, thread id and message
    boost::algorithm::replace_all(message, "%n", record.domain.getName());
    boost::algorithm::replace_all(message, "%t", boost::lexical_cast<std::string>(record.threadId));
    boost::algorithm::replace_all(message, "%m", record.message);

    return message;
}

void CLogFormatter::setPattern(const std::string & pattern)
{
    this->pattern = pattern;
}

const std::string & CLogFormatter::getPattern() const
{
    return pattern;
}

CColorMapping::CColorMapping()
{
    // Set default mappings
    auto & levelMap = map[CLoggerDomain::DOMAIN_GLOBAL];
    levelMap[ELogLevel::TRACE] = EConsoleTextColor::GRAY;
    levelMap[ELogLevel::DEBUG] = EConsoleTextColor::WHITE;
    levelMap[ELogLevel::INFO] = EConsoleTextColor::GREEN;
    levelMap[ELogLevel::WARN] = EConsoleTextColor::YELLOW;
    levelMap[ELogLevel::ERROR] = EConsoleTextColor::RED;
}

void CColorMapping::setColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level, EConsoleTextColor::EConsoleTextColor color)
{
    if(level == ELogLevel::NOT_SET) throw std::runtime_error("Log level NOT_SET not allowed for configuring the color mapping.");
    map[domain.getName()][level] = color;
}

EConsoleTextColor::EConsoleTextColor CColorMapping::getColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level) const
{
    std::string name = domain.getName();
    while(true)
    {
        const auto & loggerPair = map.find(name);
        if(loggerPair != map.end())
        {
            const auto & levelMap = loggerPair->second;
            const auto & levelPair = levelMap.find(level);
            if(levelPair != levelMap.end())
            {
                return levelPair->second;
            }
        }

        CLoggerDomain currentDomain(name);
        if(!currentDomain.isGlobalDomain())
        {
            name = currentDomain.getParent().getName();
        }
        else
        {
            break;
        }
    }
    throw std::runtime_error("No color mapping found. Should not happen.");
}

CLogConsoleTarget::CLogConsoleTarget(CConsoleHandler * console) : console(console), threshold(ELogLevel::INFO), coloredOutputEnabled(true)
{
    formatter.setPattern("%l %n [%t] - %m");
}

void CLogConsoleTarget::write(const LogRecord & record)
{
    if(threshold > record.level) return;

    std::string message = formatter.format(record);
    bool printToStdErr = record.level >= ELogLevel::WARN;
    if(console)
    {
        if(coloredOutputEnabled)
        {
            console->print(message, true, colorMapping.getColorFor(record.domain, record.level));
        }
        else
        {
            console->print(message, true, EConsoleTextColor::DEFAULT, printToStdErr);
        }
    }
    else
    {
        TLockGuard _(mx);
        if(printToStdErr)
        {
            std::cerr << message << std::endl;
        }
        else
        {
            std::cout << message << std::endl;
        }
    }
}

bool CLogConsoleTarget::isColoredOutputEnabled() const
{
    return coloredOutputEnabled;
}

void CLogConsoleTarget::setColoredOutputEnabled(bool coloredOutputEnabled)
{
    this->coloredOutputEnabled = coloredOutputEnabled;
}

ELogLevel::ELogLevel CLogConsoleTarget::getThreshold() const
{
    return threshold;
}

void CLogConsoleTarget::setThreshold(ELogLevel::ELogLevel threshold)
{
    this->threshold = threshold;
}

const CLogFormatter & CLogConsoleTarget::getFormatter() const
{
    return formatter;
}

void CLogConsoleTarget::setFormatter(const CLogFormatter & formatter)
{
    this->formatter = formatter;
}

const CColorMapping & CLogConsoleTarget::getColorMapping() const
{
    return colorMapping;
}

void CLogConsoleTarget::setColorMapping(const CColorMapping & colorMapping)
{
    this->colorMapping = colorMapping;
}

CLogFileTarget::CLogFileTarget(const std::string & filePath, bool append /*= true*/)
    : file(filePath, append ? std::ios_base::app : std::ios_base::out)
{
    formatter.setPattern("%d %l %n [%t] - %m");
}

CLogFileTarget::~CLogFileTarget()
{
    file.close();
}

void CLogFileTarget::write(const LogRecord & record)
{
    TLockGuard _(mx);
    file << formatter.format(record) << std::endl;
}

const CLogFormatter & CLogFileTarget::getFormatter() const
{
    return formatter;
}

void CLogFileTarget::setFormatter(const CLogFormatter & formatter)
{
    this->formatter = formatter;
}
