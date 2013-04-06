
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
