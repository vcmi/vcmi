
/*
 * CLogger.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class CGLogger;
struct LogRecord;
class ILogTarget;

namespace ELogLevel
{
    enum ELogLevel
    {
        NOT_SET = 0,
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR
    };
}

/// The class CLoggerDomain provides convenient access to super domains from a sub domain.
class DLL_LINKAGE CLoggerDomain
{
public:
    /// Constructs a CLoggerDomain with the domain designated by name.
    /// Sub-domains can be specified by separating domains by a dot, e.g. "ai.battle". The global domain is named "global".
    explicit CLoggerDomain(const std::string & name);

    std::string getName() const;
    CLoggerDomain getParent() const;
    bool isGlobalDomain() const;

    static const std::string DOMAIN_GLOBAL;

private:
    std::string name;
};

/// The class CLoggerStream provides a stream-like way of logging messages.
class DLL_LINKAGE CLoggerStream
{
public:
    CLoggerStream(const CGLogger & logger, ELogLevel::ELogLevel level);
    ~CLoggerStream();

    template<typename T>
    CLoggerStream & operator<<(const T & data)
    {
        if(!sbuffer) sbuffer = new std::stringstream();
        (*sbuffer) << data;
        return *this;
    }

private:
    const CGLogger & logger;
    ELogLevel::ELogLevel level;
    std::stringstream * sbuffer;
};

/// The class CGLogger is used to log messages to certain targets of a specific domain/name.
/// Temporary name is CGLogger, should be renamed to CLogger after refactoring.
class DLL_LINKAGE CGLogger
{
public:
    inline ELogLevel::ELogLevel getLevel() const;
    void setLevel(ELogLevel::ELogLevel level);
    const CLoggerDomain & getDomain() const;

    /// Logger access methods
    static CGLogger * getLogger(const CLoggerDomain & domain);
    static CGLogger * getGlobalLogger();

    /// Log methods for various log levels
    void trace(const std::string & message) const;
    CLoggerStream traceStream() const;

    void debug(const std::string & message) const;
    CLoggerStream debugStream() const;

    void info(const std::string & message) const;
    CLoggerStream infoStream() const;

    void warn(const std::string & message) const;
    CLoggerStream warnStream() const;

    void error(const std::string & message) const;
    CLoggerStream errorStream() const;

    inline void log(ELogLevel::ELogLevel level, const std::string & message) const;

    void addTarget(unique_ptr<ILogTarget> && target);
    void clearTargets();

private:
    explicit CGLogger(const CLoggerDomain & domain);
    CGLogger * getParent() const;
    inline ELogLevel::ELogLevel getEffectiveLevel() const; /// Returns the log level applied on this logger whether directly or indirectly.
    inline void callTargets(const LogRecord & record) const;

    CLoggerDomain domain;
    CGLogger * parent;
    ELogLevel::ELogLevel level;
    std::list<unique_ptr<ILogTarget> > targets;
    mutable boost::mutex mx;
    static boost::recursive_mutex smx;
};

extern DLL_LINKAGE CGLogger * logGlobal;
extern DLL_LINKAGE CGLogger * logBonus;
extern DLL_LINKAGE CGLogger * logNetwork;

/* ---------------------------------------------------------------------------- */
/* Implementation/Detail classes, Private API */
/* ---------------------------------------------------------------------------- */

/// The class CLogManager is a global storage for logger objects.
class DLL_LINKAGE CLogManager : public boost::noncopyable
{
public:
    static CLogManager & get();

    void addLogger(CGLogger * logger);
    CGLogger * getLogger(const CLoggerDomain & domain); /// Returns a logger or nullptr if no one is registered for the given domain.

private:
    CLogManager();
    ~CLogManager();

    std::map<std::string, CGLogger *> loggers;
    mutable boost::mutex mx;
    static boost::recursive_mutex smx;
};

/// The struct LogRecord holds the log message and additional logging information.
struct DLL_LINKAGE LogRecord
{
    LogRecord(const CLoggerDomain & domain, ELogLevel::ELogLevel level, const std::string & message)
        : domain(domain), level(level), message(message), timeStamp(boost::posix_time::second_clock::local_time()), threadId(boost::this_thread::get_id())
    {

    }

    CLoggerDomain domain;
    ELogLevel::ELogLevel level;
    std::string message;
    boost::posix_time::ptime timeStamp;
    boost::thread::id threadId;
};

/// The class CLogFormatter formats log records.
///
/// There are several pattern characters which can be used to format a log record:
/// %d = Date/Time
/// %l = Log level
/// %n = Logger name
/// %t = Thread ID
/// %m = Message
class DLL_LINKAGE CLogFormatter
{
public:
    CLogFormatter();
    CLogFormatter(const std::string & pattern);

    void setPattern(const std::string & pattern);
    const std::string & getPattern() const;

    std::string format(const LogRecord & record) const;

private:
    std::string pattern;
};

/// The interface ILogTarget is used by all log target implementations. It holds
/// the abstract method write which sub-classes should implement.
class DLL_LINKAGE ILogTarget : public boost::noncopyable
{
public:
    virtual ~ILogTarget() { };
    virtual void write(const LogRecord & record) = 0;
};

/// The class CColorMapping maps a logger name and a level to a specific color. Supports domain inheritance.
class DLL_LINKAGE CColorMapping
{
public:
    CColorMapping();

    void setColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level, EConsoleTextColor::EConsoleTextColor color);
    EConsoleTextColor::EConsoleTextColor getColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level) const;

private:
    std::map<std::string, std::map<ELogLevel::ELogLevel, EConsoleTextColor::EConsoleTextColor> > map;
};

/// The class CLogConsoleTarget is a logging target which writes message to the console.
class DLL_LINKAGE CLogConsoleTarget : public ILogTarget
{
public:
    explicit CLogConsoleTarget(CConsoleHandler * console);

    bool isColoredOutputEnabled() const;
    void setColoredOutputEnabled(bool coloredOutputEnabled);

    ELogLevel::ELogLevel getThreshold() const;
    void setThreshold(ELogLevel::ELogLevel threshold);

    const CLogFormatter & getFormatter() const;
    void setFormatter(const CLogFormatter & formatter);

    const CColorMapping & getColorMapping() const;
    void setColorMapping(const CColorMapping & colorMapping);

    void write(const LogRecord & record);

private:
    CConsoleHandler * console;
    ELogLevel::ELogLevel threshold;
    bool coloredOutputEnabled;
    CLogFormatter formatter;
    CColorMapping colorMapping;
    mutable boost::mutex mx;
};

/// The class CLogFileTarget is a logging target which writes messages to a log file.
class DLL_LINKAGE CLogFileTarget : public ILogTarget
{
public:
    /// Constructs a CLogFileTarget and opens the file designated by filePath. If the append parameter is true, the file will be appended to. Otherwise the file designated by filePath
    /// will be truncated before being opened.
    explicit CLogFileTarget(const std::string & filePath, bool append = true);
    ~CLogFileTarget();

    const CLogFormatter & getFormatter() const;
    void setFormatter(const CLogFormatter & formatter);

    void write(const LogRecord & record);

private:
    std::ofstream file;
    CLogFormatter formatter;
    mutable boost::mutex mx;
};
