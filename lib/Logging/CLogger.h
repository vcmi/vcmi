
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

class CGLogger;

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
