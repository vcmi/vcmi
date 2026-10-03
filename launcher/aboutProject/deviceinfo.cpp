/*
 * deviceinfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "deviceinfo.h"

#include "../../lib/GameConstants.h"

#if defined(VCMI_ANDROID)
#include <QAndroidJniObject>
#endif
#if defined(VCMI_IOS)
#include "ios/iOS_utils.h"
#endif
#if defined(VCMI_WINDOWS)
#include <QSettings>
#include <windows.h>
#elif defined(VCMI_APPLE)
#include <sys/sysctl.h>
#endif

QString DeviceInfo::formatBytes(quint64 bytes)
{
	return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

QString DeviceInfo::readProcValue(const QString & fileName, const QStringList & keys)
{
	QFile file(fileName);

	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return {};

	while(!file.atEnd())
	{
		const QString line = QString::fromUtf8(file.readLine()).trimmed();
		const int separator = line.indexOf(QLatin1Char(':'));

		if(separator < 0)
			continue;

		const QString key = line.left(separator).trimmed();

		if(keys.contains(key, Qt::CaseInsensitive))
			return line.mid(separator + 1).trimmed();
	}

	return {};
}

#if defined(VCMI_WINDOWS)
QString DeviceInfo::cpuName()
{
	QSettings cpu(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), QSettings::NativeFormat);
	return cpu.value(QStringLiteral("ProcessorNameString")).toString().trimmed();
}

std::optional<quint64> DeviceInfo::totalMemory()
{
	MEMORYSTATUSEX status{};
	status.dwLength = sizeof(status);

	if(!GlobalMemoryStatusEx(&status))
		return std::nullopt;

	return status.ullTotalPhys;
}

QString DeviceInfo::cpuFrequency()
{
	QSettings cpu(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), QSettings::NativeFormat);
	const uint frequency = cpu.value(QStringLiteral("~MHz")).toUInt();

	return frequency == 0 ? QString() : QStringLiteral("%1 MHz").arg(frequency);
}

#elif defined(VCMI_APPLE)
QByteArray DeviceInfo::sysctlData(const char * name)
{
	size_t size = 0;
	if(sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0)
		return {};

	QByteArray result(static_cast<int>(size), Qt::Uninitialized);

	if(sysctlbyname(name, result.data(), &size, nullptr, 0) != 0)
		return {};

	result.resize(static_cast<int>(size));

	return result;
}

QString DeviceInfo::cpuName()
{
	QByteArray value = sysctlData("machdep.cpu.brand_string");

	if(value.isEmpty())
		value = sysctlData("hw.machine");

	return QString::fromUtf8(value.constData()).trimmed();
}

std::optional<quint64> DeviceInfo::totalMemory()
{
	const QByteArray value = sysctlData("hw.memsize");

	if(value.size() < static_cast<int>(sizeof(quint64)))
		return std::nullopt;

	quint64 result = 0;
	std::memcpy(&result, value.constData(), sizeof(result));

	return result;
}

QString DeviceInfo::cpuFrequency()
{
	const QByteArray value = sysctlData("hw.cpufrequency");

	if(value.size() < static_cast<int>(sizeof(quint64)))
		return {};

	quint64 frequency = 0;
	std::memcpy(&frequency, value.constData(), sizeof(frequency));

	return frequency == 0 ? QString() : QStringLiteral("%1 MHz").arg(frequency / 1000000);
}

#else
QString DeviceInfo::cpuName()
{
	return readProcValue(QStringLiteral("/proc/cpuinfo"), { QStringLiteral("model name"), QStringLiteral("Hardware"), QStringLiteral("Processor") });
}

std::optional<quint64> DeviceInfo::totalMemory()
{
	const QString value = readProcValue(QStringLiteral("/proc/meminfo"), { QStringLiteral("MemTotal") });
	const auto match = QRegularExpression(QStringLiteral("^(\\d+)\\s+kB$"), QRegularExpression::CaseInsensitiveOption).match(value);

	if(!match.hasMatch())
		return std::nullopt;

	return match.captured(1).toULongLong() * 1024;
}

QString DeviceInfo::cpuFrequency()
{
	const QString value = readProcValue(QStringLiteral("/proc/cpuinfo"), { QStringLiteral("cpu MHz") });

	if(value.isEmpty())
		return {};

	return QStringLiteral("%1 MHz").arg(QLocale::c().toDouble(value), 0, 'f', 0);
}

#endif

QString DeviceInfo::generateReport()
{
	QString info;
	QTextStream stream(&info);

	stream << "VCMI version: " << QString(GameConstants::VCMI_VERSION) << '\n';
	stream << "Operating system: " << QSysInfo::prettyProductName() << '\n';
	stream << "Kernel: " << QSysInfo::kernelType() << ' ' << QSysInfo::kernelVersion() << '\n';
	stream << "CPU architecture: " << QSysInfo::currentCpuArchitecture() << '\n';
	stream << "Build ABI: " << QSysInfo::buildAbi() << '\n';

#if defined(VCMI_ANDROID)
	stream << "Device model: " << QAndroidJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;").toString() << '\n';
	stream << "Manufacturer: " << QAndroidJniObject::getStaticObjectField("android/os/Build", "MANUFACTURER", "Ljava/lang/String;").toString() << '\n';
#elif defined(VCMI_IOS)
	stream << "Device model: " << QString::fromStdString(iOS_utils::iphoneHardwareId()) << '\n';
	stream << "Manufacturer: Apple\n";
#endif

	const QString processor = cpuName();
	if(!processor.isEmpty())
		stream << "CPU: " << processor << '\n';

	stream << "Logical CPU cores: " << QThread::idealThreadCount() << '\n';
	const QString frequency = cpuFrequency();
	if(!frequency.isEmpty())
		stream << "CPU frequency (reported): " << frequency << '\n';

	if(const auto memory = totalMemory())
		stream << "System memory: " << formatBytes(*memory) << '\n';

	stream << "Qt version: " << QT_VERSION_STR << '\n';
	return info;
}
