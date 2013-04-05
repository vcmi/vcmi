#include "StdInc.h"
#include "CLogFormatter.h"

#include "LogRecord.h"

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
    boost::posix_time::time_facet * facet = new boost::posix_time::time_facet("%d-%b-%Y %H:%M:%S");
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
