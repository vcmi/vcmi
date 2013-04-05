
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
class LogRecord;
class ILogTarget;

namespace ELogLevel
{
    /**
     * The log level enum holds various log level definitions.
     */
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
     * @param domain The domain name. Sub-domains can be specified by separating domains by a dot, e.g. "ai.battle". The global domain is named "global".
     */
    CLoggerDomain(const std::string & name);

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

    /**
     * Gets the name of the domain.
     *
     * @return the name of the domain
     */
    std::string getName() const;

    /** Constant to the global domain name. */
    static const std::string DOMAIN_GLOBAL;

private:
    /** The domain name. */
    std::string name;
};

/**
 * The logger is used to log messages to certain targets of a specific domain/name. Temporary name is
 * CGLogger, should be renamed to CLogger after refactoring.
 */
class DLL_LINKAGE CGLogger : public boost::noncopyable
{
public:
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
     * Logs a message with the debug level.
     *
     * @param message The message to log.
     */
    void debug(const std::string & message) const;

    /**
     * Logs a message with the info level.
     *
     * @param message The message to log.
     */
    void info(const std::string & message) const;

    /**
     * Logs a message with the warn level.
     *
     * @param message The message to log.
     */
    void warn(const std::string & message) const;

    /**
     * Logs a message with the error level.
     *
     * @param message The message to log.
     */
    void error(const std::string & message) const;

    /**
     * Gets the log level applied for this logger. The default level for the root logger is INFO.
     *
     * @return the log level
     */
    inline ELogLevel::ELogLevel getLevel() const;

    /**
     * Sets the log level.
     *
     * @param level The log level.
     */
    void setLevel(ELogLevel::ELogLevel level);

    /**
     * Gets the logger domain.
     *
     * @return the domain of the logger
     */
    const CLoggerDomain & getDomain() const;

    /**
     * Adds a target to this logger and indirectly to all loggers which derive from this logger.
     * The logger holds strong-ownership of the target object.
     *
     * @param target The log target to add.
     */
    void addTarget(ILogTarget * target);

    /**
     * Destructor.
     */
    ~CGLogger();

private:
    /**
     * Constructor.
     *
     * @param domain The domain of the logger.
     */
    explicit CGLogger(const CLoggerDomain & domain);

    /**
     * Gets the parent logger.
     *
     * @return the parent logger or nullptr if this is the root logger
     */
    CGLogger * getParent() const;

    /**
     * Logs a message of the given log level.
     *
     * @param level The log level of the message to log.
     * @param message The message to log.
     */
    inline void log(ELogLevel::ELogLevel level, const std::string & message) const;

    /**
     * Gets the effective log level.
     *
     * @return the effective log level with respect to parent log levels
     */
    inline ELogLevel::ELogLevel getEffectiveLevel() const;

    /**
     * Calls all targets in the hierarchy to write the message.
     *
     * @param record The log record to write.
     */
    inline void callTargets(const LogRecord & record) const;

    /**
     * Gets all log targets attached to this logger.
     *
     * @return all log targets as a list
     */
    inline std::list<ILogTarget *> getTargets() const;

    /** The domain of the logger. */
    CLoggerDomain domain;

    /** A reference to the parent logger. */
    CGLogger * parent;

    /** The log level of the logger. */
    ELogLevel::ELogLevel level;

    /** A list of log targets. */
    std::list<ILogTarget *> targets;

    /** The shared mutex for providing synchronous thread-safe access to the logger. */
    mutable boost::shared_mutex mx;

    /** The unique mutex for providing thread-safe get logger access. */
    static boost::mutex smx;
};

//extern CLogger * logGlobal;
