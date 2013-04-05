
/*
 * CLogManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class CGLogger;
class CLoggerDomain;

/**
 * The log manager is a global storage of all logger objects.
 */
class DLL_LINKAGE CLogManager : public boost::noncopyable
{
public:
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

    /**
     * Destructor.
     */
    ~CLogManager();

private:
    /**
     * Constructor.
     */
    CLogManager();

    /** The instance of the log manager. */
    static CLogManager * instance;

    /** The loggers map where the key is the logger name and the value the corresponding logger. */
    std::map<std::string, CGLogger *> loggers;

    /** The shared mutex for providing synchronous thread-safe access to the log manager. */
    mutable boost::shared_mutex mx;

    /** The unique mutex for providing thread-safe singleton access. */
    static boost::mutex smx;
};
