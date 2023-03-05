/*
 * CLogger.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CLogger.h"

#ifdef VCMI_ANDROID
#include <android/log.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace ELogLevel
{
	int toAndroid(ELogLevel logLevel)
	{
		switch (logLevel)
		{
			case TRACE: return ANDROID_LOG_VERBOSE;
			case DEBUG: return ANDROID_LOG_DEBUG;
			case INFO:  return ANDROID_LOG_INFO;
			case WARN:  return ANDROID_LOG_WARN;
			case ERROR: return ANDROID_LOG_ERROR;
		default:
			break;
		}
		return ANDROID_LOG_UNKNOWN;
	}
}

VCMI_LIB_NAMESPACE_END
#elif defined(VCMI_IOS)
#import "iOS_utils.h"
extern "C" {
#include <os/log.h>
}
#endif

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

CLoggerBase::~CLoggerBase() = default;


CTraceLogger::CTraceLogger(const CLoggerBase * logger, const std::string & beginMessage, const std::string & endMessage)
	: logger(logger), endMessage(endMessage)
{
	logger->trace(beginMessage);
}

CTraceLogger::~CTraceLogger()
{
	logger->trace(endMessage);
}

}//namespace vstd

const std::string CLoggerDomain::DOMAIN_GLOBAL = "global";

CLoggerDomain::CLoggerDomain(std::string name) : name(std::move(name))
{
	if (this->name.empty())
		throw std::runtime_error("Logger domain cannot be empty.");
}

CLoggerDomain CLoggerDomain::getParent() const
{
	if(isGlobalDomain())
		return *this;

	const size_t pos = name.find_last_of(".");
	if(pos != std::string::npos)
		return CLoggerDomain(name.substr(0, pos));
	return CLoggerDomain(DOMAIN_GLOBAL);
}

bool CLoggerDomain::isGlobalDomain() const { return name == DOMAIN_GLOBAL; }

const std::string& CLoggerDomain::getName() const { return name; }

boost::recursive_mutex CLogger::smx;
boost::recursive_mutex CLogManager::smx;

DLL_LINKAGE vstd::CLoggerBase * logGlobal = CLogger::getGlobalLogger();

DLL_LINKAGE vstd::CLoggerBase * logBonus = CLogger::getLogger(CLoggerDomain("bonus"));
DLL_LINKAGE vstd::CLoggerBase * logNetwork = CLogger::getLogger(CLoggerDomain("network"));
DLL_LINKAGE vstd::CLoggerBase * logAi = CLogger::getLogger(CLoggerDomain("ai"));
DLL_LINKAGE vstd::CLoggerBase * logAnim = CLogger::getLogger(CLoggerDomain("animation"));
DLL_LINKAGE vstd::CLoggerBase * logMod = CLogger::getLogger(CLoggerDomain("mod"));

CLogger * CLogger::getLogger(const CLoggerDomain & domain)
{
	TLockGuardRec _(smx);

	CLogger * logger = CLogManager::get().getLogger(domain);
	if(!logger) // Create new logger
	{
		logger = new CLogger(domain);
		if(domain.isGlobalDomain())
		{
			logger->setLevel(ELogLevel::TRACE);
		}
		CLogManager::get().addLogger(logger);
		if (logGlobal != nullptr)
		{
			logGlobal->debug("Created logger %s", domain.getName());
		}
	}
	return logger;
}

CLogger * CLogger::getGlobalLogger()
{
	return getLogger(CLoggerDomain(CLoggerDomain::DOMAIN_GLOBAL));
}

CLogger::CLogger(const CLoggerDomain & domain) : domain(domain)
{
	if(domain.isGlobalDomain())
	{
		level = ELogLevel::TRACE;
		parent = nullptr;
	}
	else
	{
		level = ELogLevel::NOT_SET;
		parent = getLogger(domain.getParent());
	}
}

void CLogger::log(ELogLevel::ELogLevel level, const std::string & message) const
{
	if(getEffectiveLevel() <= level)
		callTargets(LogRecord(domain, level, message));
}

void CLogger::log(ELogLevel::ELogLevel level, const boost::format & fmt) const
{
	try
	{
		log(level, fmt.str());
	}
	catch(...)
	{
		log(ELogLevel::ERROR, "Invalid log format!");
	}
}

ELogLevel::ELogLevel CLogger::getLevel() const
{
	TLockGuard _(mx);
	return level;
}

void CLogger::setLevel(ELogLevel::ELogLevel level)
{
	TLockGuard _(mx);
	if (!domain.isGlobalDomain() || level != ELogLevel::NOT_SET)
		this->level = level;
}

const CLoggerDomain & CLogger::getDomain() const { return domain; }

void CLogger::addTarget(std::unique_ptr<ILogTarget> && target)
{
	TLockGuard _(mx);
	targets.push_back(std::move(target));
}

ELogLevel::ELogLevel CLogger::getEffectiveLevel() const
{
	for(const CLogger * logger = this; logger != nullptr; logger = logger->parent)
		if(logger->getLevel() != ELogLevel::NOT_SET)
			return logger->getLevel();

	// This shouldn't be reached, as the root logger must have set a log level
	return ELogLevel::INFO;
}

void CLogger::callTargets(const LogRecord & record) const
{
	TLockGuard _(mx);
	for(const CLogger * logger = this; logger != nullptr; logger = logger->parent)
		for(auto & target : logger->targets)
			target->write(record);
}

void CLogger::clearTargets()
{
	TLockGuard _(mx);
	targets.clear();
}

bool CLogger::isDebugEnabled() const { return getEffectiveLevel() <= ELogLevel::DEBUG; }
bool CLogger::isTraceEnabled() const { return getEffectiveLevel() <= ELogLevel::TRACE; }

CLogManager & CLogManager::get()
{
	TLockGuardRec _(smx);
	static CLogManager instance;
	return instance;
}

CLogManager::CLogManager() = default;
CLogManager::~CLogManager()
{
	for(auto & i : loggers)
		delete i.second;
}

void CLogManager::addLogger(CLogger * logger)
{
	TLockGuard _(mx);
	loggers[logger->getDomain().getName()] = logger;
}

CLogger * CLogManager::getLogger(const CLoggerDomain & domain)
{
	TLockGuard _(mx);
	auto it = loggers.find(domain.getName());
	if(it != loggers.end())
		return it->second;
	else
		return nullptr;
}

std::vector<std::string> CLogManager::getRegisteredDomains() const
{
	std::vector<std::string> domains;
	for (auto& pair : loggers)
	{
		domains.push_back(pair.second->getDomain().getName());
	}
	return domains;
}

CLogFormatter::CLogFormatter()
	: CLogFormatter("%m")
{
}

CLogFormatter::CLogFormatter(const std::string & pattern)
	: pattern(pattern)
{
}

CLogFormatter::CLogFormatter(const CLogFormatter & c) : pattern(c.pattern) { }
CLogFormatter::CLogFormatter(CLogFormatter && m) : pattern(std::move(m.pattern)) { }

CLogFormatter & CLogFormatter::operator=(const CLogFormatter & c)
{
	pattern = c.pattern;
	return *this;
}
CLogFormatter & CLogFormatter::operator=(CLogFormatter && m)
{
	pattern = std::move(m.pattern);
	return *this;
}

std::string CLogFormatter::format(const LogRecord & record) const
{
	std::string message = pattern;

	//Format date
//	boost::algorithm::replace_first(message, "%d", boost::posix_time::to_simple_string (record.timeStamp));

	//Format log level
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
	boost::algorithm::replace_first(message, "%l", level);

	//Format name, thread id and message
	boost::algorithm::replace_first(message, "%n", record.domain.getName());
	boost::algorithm::replace_first(message, "%t", record.threadId);
	boost::algorithm::replace_first(message, "%m", record.message);
	boost::algorithm::replace_first(message, "%c", boost::posix_time::to_simple_string(record.timeStamp));

	//return boost::to_string (boost::format("%d %d %d[%d] - %d") % dateStream.str() % level % record.domain.getName() % record.threadId % record.message);

	return message;
}

void CLogFormatter::setPattern(const std::string & pattern) { this->pattern = pattern; }
void CLogFormatter::setPattern(std::string && pattern) { this->pattern = std::move(pattern); }

const std::string & CLogFormatter::getPattern() const { return pattern; }

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
	assert(level != ELogLevel::NOT_SET);
	map[domain.getName()][level] = color;
}

EConsoleTextColor::EConsoleTextColor CColorMapping::getColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level) const
{
	CLoggerDomain currentDomain = domain;
	while(true)
	{
		const auto & loggerPair = map.find(currentDomain.getName());
		if(loggerPair != map.end())
		{
			const auto & levelMap = loggerPair->second;
			const auto & levelPair = levelMap.find(level);
			if(levelPair != levelMap.end())
				return levelPair->second;
		}

		if (currentDomain.isGlobalDomain())
			break;

		currentDomain = currentDomain.getParent();
	}

	throw std::runtime_error("failed to find color for requested domain/level pair");
}

CLogConsoleTarget::CLogConsoleTarget(CConsoleHandler * console) :
#if !defined(VCMI_MOBILE)
	console(console),
#endif
	threshold(ELogLevel::INFO), coloredOutputEnabled(true)
{
	formatter.setPattern("%m");
}

void CLogConsoleTarget::write(const LogRecord & record)
{
	if(threshold > record.level)
		return;

	std::string message = formatter.format(record);

#ifdef VCMI_ANDROID
	__android_log_write(ELogLevel::toAndroid(record.level), ("VCMI-" + record.domain.getName()).c_str(), message.c_str());
#elif defined(VCMI_IOS)
	os_log_type_t type;
	switch (record.level)
	{
	case ELogLevel::TRACE:
		type = OS_LOG_TYPE_DEBUG;
		break;
	case ELogLevel::DEBUG:
		type = OS_LOG_TYPE_DEFAULT;
		break;
	case ELogLevel::INFO:
		type = OS_LOG_TYPE_INFO;
		break;
	case ELogLevel::WARN:
		type = OS_LOG_TYPE_ERROR;
		break;
	case ELogLevel::ERROR:
		type = OS_LOG_TYPE_FAULT;
		break;
	default:
		return;
	}

	os_log_t currentLog;
	static std::unordered_map<std::string, decltype(currentLog)> logs;
	const auto& domainName = record.domain.getName();
	auto logIt = logs.find(domainName);
	if (logIt != logs.end())
		currentLog = logIt->second;
	else
	{
		currentLog = os_log_create(iOS_utils::bundleIdentifier(), domainName.c_str());
		logs.insert({domainName, currentLog});
	}
	os_log_with_type(currentLog, type, "%{public}s", message.c_str());
#else

	const bool printToStdErr = record.level >= ELogLevel::WARN;
	if(console)
	{
		const EConsoleTextColor::EConsoleTextColor textColor =
			coloredOutputEnabled ? colorMapping.getColorFor(record.domain, record.level) : EConsoleTextColor::DEFAULT;

		console->print(message, true, textColor, printToStdErr);
	}
	else
	{
		TLockGuard _(mx);
		if(printToStdErr)
			std::cerr << message << std::endl;
		else
			std::cout << message << std::endl;
	}
#endif
}

bool CLogConsoleTarget::isColoredOutputEnabled() const { return coloredOutputEnabled; }
void CLogConsoleTarget::setColoredOutputEnabled(bool coloredOutputEnabled) { this->coloredOutputEnabled = coloredOutputEnabled; }

ELogLevel::ELogLevel CLogConsoleTarget::getThreshold() const { return threshold; }
void CLogConsoleTarget::setThreshold(ELogLevel::ELogLevel threshold) { this->threshold = threshold; }

const CLogFormatter & CLogConsoleTarget::getFormatter() const { return formatter; }
void CLogConsoleTarget::setFormatter(const CLogFormatter & formatter) { this->formatter = formatter; }

const CColorMapping & CLogConsoleTarget::getColorMapping() const { return colorMapping; }
void CLogConsoleTarget::setColorMapping(const CColorMapping & colorMapping) { this->colorMapping = colorMapping; }

CLogFileTarget::CLogFileTarget(boost::filesystem::path filePath, bool append)
	: file(std::move(filePath), append ? std::ios_base::app : std::ios_base::out)
{
//	formatter.setPattern("%d %l %n [%t] - %m");
	formatter.setPattern("%l %n [%t] - %m");
}

void CLogFileTarget::write(const LogRecord & record)
{
	std::string message = formatter.format(record); //formatting is slow, do it outside the lock
	TLockGuard _(mx);
	file << message << std::endl;
}

const CLogFormatter & CLogFileTarget::getFormatter() const { return formatter; }
void CLogFileTarget::setFormatter(const CLogFormatter & formatter) { this->formatter = formatter; }

CLogFileTarget::~CLogFileTarget()
{
	file.close();
}

VCMI_LIB_NAMESPACE_END
