#include "StdInc.h"
#include "CLogConsoleTarget.h"

#include "LogRecord.h"

CColorMapping::CColorMapping()
{
    // Set default mappings
    auto & levelMap = map[""];
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
            console->print(message, colorMapping.getColorFor(record.domain, record.level));
        }
        else
        {
            console->print(message, EConsoleTextColor::DEFAULT, printToStdErr);
        }
    }
    else
    {
        TLockGuard _(mx);
        if(printToStdErr)
        {
            std::cerr << message << std::flush;
        }
        else
        {
            std::cout << message << std::flush;
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
