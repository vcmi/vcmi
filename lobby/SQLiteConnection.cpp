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

static void on_sqlite_error( sqlite3 * connection, [[maybe_unused]] int result )
{
	if ( result != SQLITE_OK )
	{
		printf( "sqlite error: %s\n", sqlite3_errmsg( connection ) );
	}

	assert( result == SQLITE_OK );
}

SQLiteStatement::SQLiteStatement( SQLiteInstance & instance, sqlite3_stmt * statement ):
	m_instance( instance ),
	m_statement( statement )
{ }

SQLiteStatement::~SQLiteStatement( )
{ 
	int result = sqlite3_finalize( m_statement );
	on_sqlite_error( m_instance.m_connection, result );
}

bool SQLiteStatement::execute( )
{ 
	int result = sqlite3_step( m_statement );

	switch ( result )
	{
		case SQLITE_DONE:
			return false;
		case SQLITE_ROW:
			return true;
		default:
			on_sqlite_error( m_instance.m_connection, result );
			return false;
	}
}

void SQLiteStatement::reset( )
{
	int result = sqlite3_reset( m_statement );
	on_sqlite_error( m_instance.m_connection, result );
}

void SQLiteStatement::clear()
{
	int result = sqlite3_clear_bindings(m_statement);
	on_sqlite_error(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle( size_t index, double const & value )
{
	int result = sqlite3_bind_double( m_statement, static_cast<int>(index), value );
	on_sqlite_error( m_instance.m_connection, result );
}

void SQLiteStatement::setBindSingle(size_t index, uint8_t const & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(index), value);
	on_sqlite_error(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle(size_t index, uint16_t const & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(index), value);
	on_sqlite_error(m_instance.m_connection, result);
}
void SQLiteStatement::setBindSingle(size_t index, uint32_t const & value)
{
	int result = sqlite3_bind_int(m_statement, static_cast<int>(index), value);
	on_sqlite_error(m_instance.m_connection, result);
}

void SQLiteStatement::setBindSingle( size_t index, int32_t const & value )
{
	int result = sqlite3_bind_int( m_statement, static_cast<int>( index ), value );
	on_sqlite_error( m_instance.m_connection, result );
}

void SQLiteStatement::setBindSingle( size_t index, int64_t const & value )
{
	int result = sqlite3_bind_int64( m_statement, static_cast<int>( index ), value );
	on_sqlite_error( m_instance.m_connection, result );
}

void SQLiteStatement::setBindSingle( size_t index, std::string const & value )
{
	int result = sqlite3_bind_text( m_statement, static_cast<int>( index ), value.data(), static_cast<int>( value.size() ), SQLITE_STATIC );
	on_sqlite_error( m_instance.m_connection, result );
}

void SQLiteStatement::setBindSingle( size_t index, char const * value )
{
	int result = sqlite3_bind_text( m_statement, static_cast<int>( index ), value, -1, SQLITE_STATIC );
	on_sqlite_error( m_instance.m_connection, result );
}

void SQLiteStatement::getColumnSingle( size_t index, double & value )
{
	value = sqlite3_column_double( m_statement, static_cast<int>( index ) );
}

void SQLiteStatement::getColumnSingle( size_t index, uint8_t & value )
{
	value = static_cast<uint8_t>(sqlite3_column_int( m_statement, static_cast<int>( index ) ));
}

void SQLiteStatement::getColumnSingle(size_t index, uint16_t & value)
{
	value = static_cast<uint16_t>(sqlite3_column_int(m_statement, static_cast<int>(index)));
}

void SQLiteStatement::getColumnSingle( size_t index, int32_t & value )
{
	value = sqlite3_column_int( m_statement, static_cast<int>( index ) );
}

void SQLiteStatement::getColumnSingle(size_t index, uint32_t & value)
{
	value = sqlite3_column_int(m_statement, static_cast<int>(index));
}

void SQLiteStatement::getColumnSingle( size_t index, int64_t & value )
{
	value = sqlite3_column_int64( m_statement, static_cast<int>( index ) );
}

void SQLiteStatement::getColumnSingle( size_t index, std::string & value )
{
	auto value_raw = sqlite3_column_text(m_statement, static_cast<int>(index));
	value = reinterpret_cast<char const*>( value_raw );
}

void SQLiteStatement::getColumnBlob(size_t index, std::byte * value, [[maybe_unused]] size_t capacity)
{
	auto * blob_data = sqlite3_column_blob(m_statement, static_cast<int>(index));
	size_t blob_size = sqlite3_column_bytes(m_statement, static_cast<int>(index));

	assert(blob_size < capacity);

	std::copy_n(static_cast<std::byte const*>(blob_data), blob_size, value);
	value[blob_size] = std::byte(0);
}

SQLiteInstancePtr SQLiteInstance::open( std::string const & db_path, bool allow_write)
{
	int flags = allow_write ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) : SQLITE_OPEN_READONLY;

	sqlite3 * connection;
	int result = sqlite3_open_v2( db_path.c_str( ), &connection, flags, nullptr );

	on_sqlite_error( connection, result );

	assert(result == SQLITE_OK);

	if ( result == SQLITE_OK )
		return SQLiteInstancePtr( new SQLiteInstance( connection ) );

	sqlite3_close( connection );
	return SQLiteInstancePtr( );
}

SQLiteInstance::SQLiteInstance( sqlite3 * connection ):
	m_connection( connection )
{ }

SQLiteInstance::~SQLiteInstance( )
{
	int result = sqlite3_close( m_connection );
	on_sqlite_error( m_connection, result );
}

SQLiteStatementPtr SQLiteInstance::prepare( std::string const & sql_text )
{
	sqlite3_stmt * statement;
	int result = sqlite3_prepare_v2( m_connection, sql_text.data(), static_cast<int>( sql_text.size()), &statement, nullptr );
	
	on_sqlite_error( m_connection, result );

	if ( result == SQLITE_OK )
		return SQLiteStatementPtr( new SQLiteStatement( *this, statement ) );
	sqlite3_finalize( statement );
	return SQLiteStatementPtr( );
}
