/*
 * SQLiteConnection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

using sqlite3 = struct sqlite3;
using sqlite3_stmt = struct sqlite3_stmt;

class SQLiteInstance;
class SQLiteStatement;

using SQLiteInstancePtr = std::unique_ptr<SQLiteInstance>;
using SQLiteStatementPtr = std::unique_ptr<SQLiteStatement>;

class SQLiteStatement : boost::noncopyable
{
public:
	friend class SQLiteInstance;

	bool execute();
	void reset();
	void clear();

	~SQLiteStatement();

	template<typename... Args>
	void executeOnce(const Args &... args)
	{
		setBinds(args...);
		execute();
		reset();
	}

	template<typename... Args>
	void setBinds(const Args &... args)
	{
		setBindSingle(1, args...); // The leftmost SQL parameter has an index of 1
	}

	template<typename... Args>
	void getColumns(Args &... args)
	{
		getColumnSingle(0, args...); // The leftmost column of the result set has the index 0
	}

private:
	void setBindSingle(size_t index, const double & value);
	void setBindSingle(size_t index, const bool & value);
	void setBindSingle(size_t index, const uint8_t & value);
	void setBindSingle(size_t index, const uint16_t & value);
	void setBindSingle(size_t index, const uint32_t & value);
	void setBindSingle(size_t index, const int32_t & value);
	void setBindSingle(size_t index, const int64_t & value);
	void setBindSingle(size_t index, const std::string & value);
	void setBindSingle(size_t index, const char * value);

	void getColumnSingle(size_t index, double & value);
	void getColumnSingle(size_t index, bool & value);
	void getColumnSingle(size_t index, uint8_t & value);
	void getColumnSingle(size_t index, uint16_t & value);
	void getColumnSingle(size_t index, uint32_t & value);
	void getColumnSingle(size_t index, int32_t & value);
	void getColumnSingle(size_t index, int64_t & value);
	void getColumnSingle(size_t index, std::string & value);

	template < typename T, typename std::enable_if_t < std::is_enum_v<T>, int  > = 0 >
	void getColumnSingle(size_t index, T & value)
	{
		using Integer = std::underlying_type_t<T>;
		Integer result;
		getColumnSingle(index, result);

		value = static_cast<T>(result);
	}

	template<typename Rep, typename Period>
	void getColumnSingle(size_t index, std::chrono::duration<Rep, Period> & value)
	{
		int64_t durationValue = 0;
		getColumnSingle(index, durationValue);
		value = std::chrono::duration<Rep, Period>(durationValue);
	}

	SQLiteStatement(SQLiteInstance & instance, sqlite3_stmt * statement);

	template<typename T, typename... Args>
	void setBindSingle(size_t index, T const & arg, const Args &... args)
	{
		setBindSingle(index, arg);
		setBindSingle(index + 1, args...);
	}

	template<typename T, typename... Args>
	void getColumnSingle(size_t index, T & arg, Args &... args)
	{
		getColumnSingle(index, arg);
		getColumnSingle(index + 1, args...);
	}

	SQLiteInstance & m_instance;
	sqlite3_stmt * m_statement;
};

class SQLiteInstance : boost::noncopyable
{
public:
	friend class SQLiteStatement;

	static SQLiteInstancePtr open(const boost::filesystem::path & db_path, bool allow_write);

	~SQLiteInstance();

	SQLiteStatementPtr prepare(const std::string & statement);

private:
	explicit SQLiteInstance(sqlite3 * connection);

	sqlite3 * m_connection;
};
