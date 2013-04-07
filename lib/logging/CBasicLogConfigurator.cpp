#include "StdInc.h"
#include "CBasicLogConfigurator.h"

#include "../CConfigHandler.h"

CBasicLogConfigurator::CBasicLogConfigurator(const std::string & filePath, CConsoleHandler * console)
{
    const JsonNode & logging = settings["logging"];

    // Configure loggers
    const JsonNode & loggers = logging["loggers"];
    if(!loggers.isNull())
    {
        BOOST_FOREACH(auto & loggerPair, loggers.Struct())
        {
            // Get logger
            std::string name = loggerPair.first;
            CGLogger * logger = CGLogger::getLogger(name);

            // Set log level
            const JsonNode & loggerNode = loggerPair.second;
            const JsonNode & levelNode = loggerNode["level"];
            if(!levelNode.isNull())
            {
                logger->setLevel(getLogLevel(levelNode.String()));
            }
        }
    }

    // Add console target
    CLogConsoleTarget * consoleTarget = new CLogConsoleTarget(console);
    const JsonNode & consoleNode = logging["console"];
    if(!consoleNode.isNull())
    {
        const JsonNode & consoleFormatNode = consoleNode["format"];
        if(!consoleFormatNode.isNull()) consoleTarget->setFormatter(CLogFormatter(consoleFormatNode.String()));
        const JsonNode & consoleThresholdNode = consoleNode["threshold"];
        if(!consoleThresholdNode.isNull()) consoleTarget->setThreshold(getLogLevel(consoleThresholdNode.String()));
        const JsonNode & coloredConsoleEnabledNode = consoleNode["coloredOutputEnabled"];
        consoleTarget->setColoredOutputEnabled(coloredConsoleEnabledNode.Bool());

        CColorMapping colorMapping;
        const JsonNode & colorMappingNode = consoleNode["colorMapping"];
        if(!colorMappingNode.isNull())
        {
            BOOST_FOREACH(const JsonNode & mappingNode, colorMappingNode.Vector())
            {
                std::string domain = mappingNode["domain"].String();
                std::string level = mappingNode["level"].String();
                std::string color = mappingNode["color"].String();
                colorMapping.setColorFor(domain, getLogLevel(level), getConsoleColor(color));
            }
        }
        consoleTarget->setColorMapping(colorMapping);
    }
    CGLogger::getGlobalLogger()->addTarget(consoleTarget);

    // Add file target
    CLogFileTarget * fileTarget = new CLogFileTarget(filePath);
    const JsonNode & fileNode = logging["file"];
    if(!fileNode.isNull())
    {
        const JsonNode & fileFormatNode = fileNode["format"];
        if(!fileFormatNode.isNull()) fileTarget->setFormatter(CLogFormatter(fileFormatNode.String()));
    }

    // Add targets to the root logger by default
    CGLogger::getGlobalLogger()->addTarget(consoleTarget);
    CGLogger::getGlobalLogger()->addTarget(fileTarget);
}

ELogLevel::ELogLevel CBasicLogConfigurator::getLogLevel(const std::string & level) const
{
    static const std::map<std::string, ELogLevel::ELogLevel> levelMap = boost::assign::map_list_of
            ("trace", ELogLevel::TRACE)
            ("debug", ELogLevel::DEBUG)
            ("info", ELogLevel::INFO)
            ("warn", ELogLevel::WARN)
            ("error", ELogLevel::ERROR);
    const auto & levelPair = levelMap.find(level);
    if(levelPair != levelMap.end())
    {
        return levelPair->second;
    }
    else
    {
        throw std::runtime_error("Log level " + level + " unknown.");
    }
}

EConsoleTextColor::EConsoleTextColor CBasicLogConfigurator::getConsoleColor(const std::string & colorName) const
{
    static const std::map<std::string, EConsoleTextColor::EConsoleTextColor> colorMap = boost::assign::map_list_of
            ("default", EConsoleTextColor::DEFAULT)
            ("green", EConsoleTextColor::GREEN)
            ("red", EConsoleTextColor::RED)
            ("magenta", EConsoleTextColor::MAGENTA)
            ("yellow", EConsoleTextColor::YELLOW)
            ("white", EConsoleTextColor::WHITE)
            ("gray", EConsoleTextColor::GRAY)
            ("teal", EConsoleTextColor::TEAL);
    const auto & colorPair = colorMap.find(colorName);
    if(colorPair != colorMap.end())
    {
        return colorPair->second;
    }
    else
    {
        throw std::runtime_error("Color " + colorName + " unknown.");
    }
}
