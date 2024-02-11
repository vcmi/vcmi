/*
 * SQLiteConnection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SQLiteConnection.h"

#include <sqlite3.h>

[[noreturn]] static void handleSQLiteError(sqlite3 * connection)
{
	const char * message = sqlite3_errmsg(connection);
	throw std::runtime_error(std::string("SQLite error: ") + message);
}

static void checkSQLiteError(sqlite3 * connection, int result)
{
	if(result != SQLITE_OK)
		handleSQLiteError(connection);
}

SQLiteStatement::SQLiteStatement(SQLiteInstance & instance, sqlite3_stmt * statement)
	: m_instance(instance)
	, m_statement(statement)
{
}

SQLiteStatement::~SQLiteStatement()
{
	sqlite3_finalize(m_statement);
}

bool SQLiteStatement::execute()
{
	int result = sqlite3_step(m_statement);

	switch(result)
	{
		case SQLITE_DONE:
			return false;
		case SQLITE_ROW:
			return true;
		default:
			checkSQLiteError(m_instance.m_connection, result);
			return false;
	}
}

void SQLiteStatement::reset()
{
	int result = sqlite3_reset(m_statement);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::clear()
{
	int result = sqlite3_clear_bindings(m_statement);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const double & value)
{
	int result = sqlite3_bind_double(m_statement, static_cast<int>(index), value);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const bool & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(value), value);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const uint8_t & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(index), value);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const uint16_t & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(index), value);
	checkSQLiteError(m_instance.m_connection, result);
}
void SQLiteStatement::setBindSingle(size_t index, const uint32_t & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(index), value);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const int32_t & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(index), value);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const int64_t & value)
{
	int result = sqlite3_bind_int64(m_statement, static_cast<int>(index), value);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const std::string & value)
{
	int result = sqlite3_bind_text(m_statement, static_cast<int>(index), value.data(), static_cast<int>(value.size()), SQLITE_STATIC);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, const char * value)
{
	int result = sqlite3_bind_text(m_statement, static_cast<int>(index), value, -1, SQLITE_STATIC);
	checkSQLiteError(m_instance.m_connection, result);
}

void SQLiteStatement::getColumnSingle(size_t index, double & value)
{
	value = sqlite3_column_double(m_statement, static_cast<int>(index));
}

void SQLiteStatement::getColumnSingle(size_t index, bool & value)
{
	value = sqlite3_column_int(m_statement, static_cast<int>(index)) != 0;
}

void SQLiteStatement::getColumnSingle(size_t index, uint8_t & value)
{
	value = static_cast<uint8_t>(sqlite3_column_int(m_statement, static_cast<int>(index)));
}

void SQLiteStatement::getColumnSingle(size_t index, uint16_t & value)
{
	value = static_cast<uint16_t>(sqlite3_column_int(m_statement, static_cast<int>(index)));
}

void SQLiteStatement::getColumnSingle(size_t index, int32_t & value)
{
	value = sqlite3_column_int(m_statement, static_cast<int>(index));
}

void SQLiteStatement::getColumnSingle(size_t index, uint32_t & value)
{
	value = sqlite3_column_int(m_statement, static_cast<int>(index));
}

void SQLiteStatement::getColumnSingle(size_t index, int64_t & value)
{
	value = sqlite3_column_int64(m_statement, static_cast<int>(index));
}

void SQLiteStatement::getColumnSingle(size_t index, std::string & value)
{
	const auto * value_raw = sqlite3_column_text(m_statement, static_cast<int>(index));
	value = reinterpret_cast<const char *>(value_raw);
}

SQLiteInstancePtr SQLiteInstance::open(const boost::filesystem::path & db_path, bool allow_write)
{
	int flags = allow_write ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) : SQLITE_OPEN_READONLY;

	sqlite3 * connection;
	int result = sqlite3_open_v2(db_path.c_str(), &connection, flags, nullptr);

	if(result == SQLITE_OK)
		return SQLiteInstancePtr(new SQLiteInstance(connection));

	sqlite3_close(connection);
	handleSQLiteError(connection);
}

SQLiteInstance::SQLiteInstance(sqlite3 * connection)
	: m_connection(connection)
{
}

SQLiteInstance::~SQLiteInstance()
{
	sqlite3_close(m_connection);
}

SQLiteStatementPtr SQLiteInstance::prepare(const std::string & sql_text)
{
	sqlite3_stmt * statement;
	int result = sqlite3_prepare_v2(m_connection, sql_text.data(), static_cast<int>(sql_text.size()), &statement, nullptr);

	if(result == SQLITE_OK)
		return SQLiteStatementPtr(new SQLiteStatement(*this, statement));

	sqlite3_finalize(statement);
	handleSQLiteError(m_connection);
}
