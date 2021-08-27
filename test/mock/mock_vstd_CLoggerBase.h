/*
 * mock_vstd_CLoggerBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once


class LoggerMock : public vstd::CLoggerBase
{
public:
	void log(ELogLevel::ELogLevel level, const std::string & message) const override
	{
		//TODO: save last few normal messages and add to gtest error if VCMI error logged
		switch(level)
		{
		case ELogLevel::ERROR:
		case ELogLevel::WARN:
			ADD_FAILURE() << message;
			break;
		case ELogLevel::INFO:
			break;
		case ELogLevel::DEBUG:
			break;
		case ELogLevel::TRACE:
			break;
		}
	}

	void log(ELogLevel::ELogLevel level, const boost::format & fmt) const override
	{
		this->log(level, fmt.str());
	}

	bool isDebugEnabled() const override {return true;}
	bool isTraceEnabled() const override {return true;}
};

