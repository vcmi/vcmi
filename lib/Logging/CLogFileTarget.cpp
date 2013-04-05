#include "StdInc.h"
#include "CLogFileTarget.h"

CLogFileTarget::CLogFileTarget(const std::string & filePath) : file(filePath)
{

}

CLogFileTarget::~CLogFileTarget()
{
    file.close();
}

void CLogFileTarget::write(const LogRecord & record)
{
    TLockGuard _(mx);
    file << formatter.format(record) << std::endl;
}

const CLogFormatter & CLogFileTarget::getFormatter() const
{
    return formatter;
}

void CLogFileTarget::setFormatter(const CLogFormatter & formatter)
{
    this->formatter = formatter;
}
