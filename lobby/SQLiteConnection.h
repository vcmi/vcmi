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

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

class SQLiteInstance;
class SQLiteStatement;

using SQLiteInstancePtr = std::unique_ptr< SQLiteInstance >;
using SQLiteStatementPtr = std::unique_ptr< SQLiteStatement >;

class SQLiteStatement : boost::noncopyable
{
public:
	friend class SQLiteInstance;

	bool execute( );
	void reset( );
	void clear( );

	~SQLiteStatement();

	template<typename ... Args >
	void setBinds( Args const & ... args )
	{
		setBindSingle( 1, args... ); // The leftmost SQL parameter has an index of 1
	}

	template<typename ... Args >
	void getColumns( Args & ... args )
	{
		getColumnSingle( 0, args... ); // The leftmost column of the result set has the index 0
	}

private:
	void setBindSingle( size_t index, double const & value );
	void setBindSingle( size_t index, uint8_t const & value );
	void setBindSingle( size_t index, uint16_t const & value );
	void setBindSingle( size_t index, uint32_t const & value );
	void setBindSingle( size_t index, int32_t const & value );
	void setBindSingle( size_t index, int64_t const & value );
	void setBindSingle( size_t index, std::string const & value );
	void setBindSingle( size_t index, char const * value );

	void getColumnSingle( size_t index, double & value );
	void getColumnSingle( size_t index, uint8_t & value );
	void getColumnSingle( size_t index, uint16_t & value );
	void getColumnSingle( size_t index, uint32_t & value );
	void getColumnSingle( size_t index, int32_t & value );
	void getColumnSingle( size_t index, int64_t & value );
	void getColumnSingle( size_t index, std::string & value );

	SQLiteStatement( SQLiteInstance & instance, sqlite3_stmt * statement );

	template<typename T, typename ... Args >
	void setBindSingle( size_t index, T const & arg, Args const & ... args )
	{
		setBindSingle( index, arg );
		setBindSingle( index + 1, args... );
	}

	template<typename T, typename ... Args >
	void getColumnSingle( size_t index, T & arg, Args & ... args )
	{
		getColumnSingle( index, arg );
		getColumnSingle( index + 1, args... );
	}

	void getColumnBlob(size_t index, std::byte * value, size_t capacity);

	SQLiteInstance & m_instance;
	sqlite3_stmt * m_statement;
};

class SQLiteInstance : boost::noncopyable
{
public:
	friend class SQLiteStatement;

	static SQLiteInstancePtr open(std::string const & db_path, bool allow_write );

	~SQLiteInstance( );

	SQLiteStatementPtr prepare( std::string const & statement );

private:
	SQLiteInstance( sqlite3 * connection );

	sqlite3 * m_connection;
};
