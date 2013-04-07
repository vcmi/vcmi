
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

/**
 * The logger domain provides convenient access to super domains from a sub domain.
 */
class DLL_LINKAGE CLoggerDomain
{
public:
    /**
     * Constructor.
     *
     * @param name The domain name. Sub-domains can be specified by separating domains by a dot, e.g. "ai.battle". The global domain is named "global".
     */
    CLoggerDomain(const std::string & name);

    // Accessors

    std::string getName() const;

    // Methods

    /**
     * Gets the parent logger domain.
     *
     * @return the parent logger domain or the same domain logger if this method has been called on the global domain
     */
    CLoggerDomain getParent() const;

    /**
     * Returns true if this domain is the global domain.
     *
     * @return true if this is the global domain or false if not
     */
    bool isGlobalDomain() const;

    // Constants

    /** Constant to the global domain name. */
    static const std::string DOMAIN_GLOBAL;

private:
    std::string name;
};

/**
 * The logger stream provides a stream-like way of logging messages.
 */
class DLL_LINKAGE CLoggerStream
{
public:
    /**
     * Constructs a new logger stream.
     *
     * @param logger The logger which should be used to log the generated message to.
     * @param level The log level of the generated message.
     */
    CLoggerStream(const CGLogger & logger, ELogLevel::ELogLevel level);
    ~CLoggerStream();

    // Methods

    /**
     * Writes data to the logger stream.
     *
     * @param data Any data can be written to the stream.
     */
    template<typename T>
    CLoggerStream & operator<<(const T & data)
    {
        if(!sbuffer) sbuffer = new std::stringstream();
        (*sbuffer) << data;
        return *this;
    }

private:
    // Data members

    const CGLogger & logger;
    ELogLevel::ELogLevel level;
    std::stringstream * sbuffer;
};

/**
 * The logger is used to log messages to certain targets of a specific domain/name. Temporary name is
 * CGLogger, should be renamed to CLogger after refactoring.
 */
class DLL_LINKAGE CGLogger : public boost::noncopyable
{
public:
    ~CGLogger();

    // Accessors

    inline ELogLevel::ELogLevel getLevel() const;
    void setLevel(ELogLevel::ELogLevel level);

    const CLoggerDomain & getDomain() const;

    // Methods

    /**
     * Gets a logger by domain.
     *
     * @param domain The logger domain.
     * @return the logger object
     */
    static CGLogger * getLogger(const CLoggerDomain & domain);

    /**
     * Gets the global logger which is the parent of all domain-specific loggers.
     *
     * @return the global logger object
     */
    static CGLogger * getGlobalLogger();

    /**
     * Logs a message with the trace level.
     *
     * @param message The message to log.
     */
    void trace(const std::string & message) const;

    /**
     * Returns a logger stream with the trace level.
     *
     * @return the logger stream
     */
    CLoggerStream traceStream() const;

    /**
     * Logs a message with the debug level.
     *
     * @param message The message to log.
     */
    void debug(const std::string & message) const;

    /**
     * Returns a logger stream with the debug level.
     *
     * @return the logger stream
     */
    CLoggerStream debugStream() const;

    /**
     * Logs a message with the info level.
     *
     * @param message The message to log.
     */
    void info(const std::string & message) const;

    /**
     * Returns a logger stream with the info level.
     *
     * @return the logger stream
     */
    CLoggerStream infoStream() const;

    /**
     * Logs a message with the warn level.
     *
     * @param message The message to log.
     */
    void warn(const std::string & message) const;

    /**
     * Returns a logger stream with the warn level.
     *
     * @return the logger stream
     */
    CLoggerStream warnStream() const;

    /**
     * Logs a message with the error level.
     *
     * @param message The message to log.
     */
    void error(const std::string & message) const;

    /**
     * Returns a logger stream with the error level.
     *
     * @return the logger stream
     */
    CLoggerStream errorStream() const;

    /**
     * Logs a message of the given log level.
     *
     * @param level The log level of the message to log.
     * @param message The message to log.
     */
    inline void log(ELogLevel::ELogLevel level, const std::string & message) const;

    /**
     * Adds a target to this logger and indirectly to all loggers which derive from this logger.
     * The logger holds strong-ownership of the target object.
     *
     * @param target The log target to add.
     */
    void addTarget(ILogTarget * target);

private:
    // Methods

    explicit CGLogger(const CLoggerDomain & domain);
    CGLogger * getParent() const;
    inline ELogLevel::ELogLevel getEffectiveLevel() const;
    inline void callTargets(const LogRecord & record) const;
    inline std::list<ILogTarget *> getTargets() const;

    // Data members

    CLoggerDomain domain;
    CGLogger * parent;
    ELogLevel::ELogLevel level;
    std::list<ILogTarget *> targets;
    mutable boost::shared_mutex mx;
    static boost::mutex smx;
};

/* ---------------------------------------------------------------------------- */
/* Implementation/Detail classes, Private API */
/* ---------------------------------------------------------------------------- */

/**
 * The log manager is a global storage of all logger objects.
 */
class DLL_LINKAGE CLogManager : public boost::noncopyable
{
public:
    ~CLogManager();

    // Methods

    /**
     * Gets an instance of the log manager.
     *
     * @return an instance of the log manager
     */
    static CLogManager * get();

    /**
     * Adds a logger. The log manager holds strong ownership of the logger object.
     *
     * @param logger The logger to add.
     */
    void addLogger(CGLogger * logger);

    /**
     * Gets a logger by domain.
     *
     * @param domain The domain of the logger.
     * @return a logger by domain or nullptr if the logger was not found
     */
    CGLogger * getLogger(const CLoggerDomain & domain);

private:
    // Methods

    CLogManager();

    // Data members

    static CLogManager * instance;
    std::map<std::string, CGLogger *> loggers;
    mutable boost::shared_mutex mx;
    static boost::mutex smx;
};

/**
 * The log records holds the log message and additional logging information.
 */
struct DLL_LINKAGE LogRecord
{
    LogRecord(const CLoggerDomain & domain, ELogLevel::ELogLevel level, const std::string & message)
        : domain(domain), level(level), message(message), timeStamp(boost::posix_time::second_clock::local_time()), threadId(boost::this_thread::get_id())
    {

    }

    /** The logger domain. */
    CLoggerDomain domain;

    /** The log level. */
    ELogLevel::ELogLevel level;

    /** The message. */
    std::string message;

    /** The time when the message was created. */
    boost::posix_time::ptime timeStamp;

    /** The thread id. */
    boost::thread::id threadId;
};

/**
 * The log formatter formats log records.
 *
 * There are several pattern characters which can be used to format a log record:
 * %d = Date/Time
 * %l = Log level
 * %n = Logger name
 * %t = Thread ID
 * %m = Message
 */
class DLL_LINKAGE CLogFormatter
{
public:
    CLogFormatter();

    /**
     * Constructor.
     *
     * @param pattern The pattern to format the log record with.
     */
    CLogFormatter(const std::string & pattern);

    // Accessors

    void setPattern(const std::string & pattern);
    const std::string & getPattern() const;

    // Methods

    /**
     * Formats a log record.
     *
     * @param record The log record to format.
     * @return the formatted log record as a string
     */
    std::string format(const LogRecord & record) const;

private:
    // Data members

    std::string pattern;
};

/**
 * The interface log target is used by all log target implementations. It holds
 * the abstract method write which sub-classes should implement.
 */
class DLL_LINKAGE ILogTarget : public boost::noncopyable
{
public:
    virtual ~ILogTarget() { };

    /**
     * Writes a log record.
     *
     * @param record The log record to write.
     */
    virtual void write(const LogRecord & record) = 0;
};

/**
 * The color mapping maps a logger name and a level to a specific color.
 */
class DLL_LINKAGE CColorMapping
{
public:
    /**
     * Constructor. There are default color mappings for the root logger, which child loggers inherit if not overriden.
     */
    CColorMapping();

    // Methods

    /**
     * Sets a console text color for a logger name and a level.
     *
     * @param domain The domain of the logger.
     * @param level The logger level.
     * @param color The console text color to use as the mapping.
     */
    void setColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level, EConsoleTextColor::EConsoleTextColor color);

    /**
     * Gets a console text color for a logger name and a level.
     *
     * @param domain The domain of the logger.
     * @param level The logger level.
     * @return the console text color which has been applied for the mapping
     */
    EConsoleTextColor::EConsoleTextColor getColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level) const;

private:
    // Data members

    std::map<std::string, std::map<ELogLevel::ELogLevel, EConsoleTextColor::EConsoleTextColor> > map;
};

/**
 * The console target is a logging target which writes message to the console.
 */
class DLL_LINKAGE CLogConsoleTarget : public ILogTarget
{
public:
    /**
     * Constructor.
     *
     * @param console Optional. The console handler which is used to output messages to the console.
     */
    explicit CLogConsoleTarget(CConsoleHandler * console);

    // Accessors

    bool isColoredOutputEnabled() const;
    void setColoredOutputEnabled(bool coloredOutputEnabled);

    ELogLevel::ELogLevel getThreshold() const;
    void setThreshold(ELogLevel::ELogLevel threshold);

    const CLogFormatter & getFormatter() const;
    void setFormatter(const CLogFormatter & formatter);

    const CColorMapping & getColorMapping() const;
    void setColorMapping(const CColorMapping & colorMapping);

    // Methods

    /**
     * Writes a log record to the console.
     *
     * @param record The log record to write.
     */
    void write(const LogRecord & record);

private:
    // Data members

    CConsoleHandler * console;
    ELogLevel::ELogLevel threshold;
    bool coloredOutputEnabled;
    CLogFormatter formatter;
    CColorMapping colorMapping;
    mutable boost::mutex mx;
};

/**
 * The log file target is a logging target which writes messages to a log file.
 */
class DLL_LINKAGE CLogFileTarget : public ILogTarget
{
public:
    /**
     * Constructor.
     *
     * @param filePath The file path of the log file.
     */
    explicit CLogFileTarget(const std::string & filePath);
    ~CLogFileTarget();

    // Accessors

    const CLogFormatter & getFormatter() const;
    void setFormatter(const CLogFormatter & formatter);

    // Methods

    /**
     * Writes a log record to the log file.
     *
     * @param record The log record to write.
     */
    void write(const LogRecord & record);

private:
    // Data members

    std::ofstream file;
    CLogFormatter formatter;
    mutable boost::mutex mx;
};
