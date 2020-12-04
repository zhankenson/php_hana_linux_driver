#ifndef CORE_HDB_H
#define CORE_HDB_H

//---------------------------------------------------------------------------------------------------------------------------------
// File: core_hdb.h
//
// Contents: Core routines and constants shared by the Microsoft Drivers for PHP for SQL Server
//
//---------------------------------------------------------------------------------------------------------------------------------

//*********************************************************************************************************************************
// Includes
//*********************************************************************************************************************************

#ifdef SQL_WCHART_CONVERT
#undef SQL_WCHART_CONVERT
#endif
#ifndef _WCHART_DEFINED
#define _WCHART_DEFINED 
#endif

#include "php.h"
#include "php_globals.h"
#include "php_ini.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"

#ifndef _WIN32 // !_WIN32
#include "FormattedPrint.h"
#include "StringFunctions.h"
#else
#include "VersionHelpers.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef PHP_WIN32
#define PHP_HDB_API __declspec(dllexport)
#else
#define PHP_HDB_API
#endif

// #define MultiByteToWideChar SystemLocale::ToUtf16

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define strnlen_s(s) strnlen_s(s, INT_MAX)

#ifndef _WIN32
#define GetLastError() errno
#define SetLastError(err) errno=err

typedef struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    CHAR   szCSDVersion[ 128 ];     // Maintenance string for PSS usage
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;
typedef OSVERSIONINFOA OSVERSIONINFO;
#endif // !_WIN32


// OACR is an internal Microsoft static code analysis tool
#if defined(OACR)
#include <oacr.h>
OACR_WARNING_PUSH
OACR_WARNING_DISABLE( ALLOC_SIZE_OVERFLOW, "Third party code." )
OACR_WARNING_DISABLE( INDEX_NEGATIVE, "Third party code." )
OACR_WARNING_DISABLE( UNANNOTATED_BUFFER, "Third party code." )
OACR_WARNING_DISABLE( INDEX_UNDERFLOW, "Third party code." )
OACR_WARNING_DISABLE( REALLOCLEAK, "Third party code." )
OACR_WARNING_DISABLE( ALLOC_SIZE_OVERFLOW_WITH_ACCESS, "Third party code." )
#else
// define to eliminate static analysis hints in the code
#define OACR_WARNING_SUPPRESS( warning, msg )
#endif

extern "C" {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning( disable: 4005 4100 4127 4142 4244 4505 4530 )
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#if _MSC_VER >= 1400
// typedef and macro to prevent a conflict between php.h and ws2tcpip.h.  
// php.h defines this  constant as unsigned int which causes a compile error 
// in ws2tcpip.h.  Fortunately php.h allows an override by defining
// HAVE_SOCKLEN_T.  Since ws2tcpip.h isn't included until later, we define 
// socklen_t here and override the php.h version.
typedef int socklen_t;
#define HAVE_SOCKLEN_T
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if ZEND_DEBUG
// debug build causes warning C4505 to pop up from the Zend header files
#pragma warning( disable: 4505 )
#endif

}   // extern "C"

#if defined(OACR)
OACR_WARNING_POP
#endif

#ifdef _WIN32
#include <sql.h>
#include "common/odbc/sqlext.h"
#endif // _WIN32

#if !defined(SQL_GUID)
// imported from sqlext.h
#define SQL_GUID            (-11)
#endif

#if !defined(WC_ERR_INVALID_CHARS)
// imported from winnls.h as it isn't included by 5.3.0
#define WC_ERR_INVALID_CHARS      0x00000080  // error for invalid chars
#endif

// PHP defines inline as __forceinline, which in debug mode causes a warning to be emitted when 
// we use std::copy, which causes compilation to fail since we compile with warnings as errors.
#if defined(ZEND_DEBUG) && defined(inline)
#undef inline
#endif

#include <deque>
#include <map>
#include <string>
#include <algorithm>
#include <limits>
#include <cassert>
#include <memory>
#include <vector>
// included for HANA specific constants
#include "common/odbc/sqlext.h"
#ifdef _WIN32
#include <strsafe.h>
#endif // _WIN32

//*********************************************************************************************************************************
// Constants and Types
//*********************************************************************************************************************************

// constants for maximums in SQL Server
const int SS_MAXCOLNAMELEN = 128;
const int SQL_SERVER_MAX_FIELD_SIZE = 8000;
const int SQL_SERVER_MAX_PRECISION = 38;
const int SQL_SERVER_MAX_TYPE_SIZE = 0;
const int SQL_SERVER_MAX_PARAMS = 2100;
// increase the maximum message length to accommodate for the long error returned for operand type clash
// or for conversion of a long string
const int SQL_MAX_ERROR_MESSAGE_LENGTH = SQL_MAX_MESSAGE_LENGTH * 8;

// max size of a date time string when converting from a DateTime object to a string
const int MAX_DATETIME_STRING_LEN = 256;

// precision and scale for the date time types between servers
const int SQL_SERVER_2005_DEFAULT_DATETIME_PRECISION = 23;
const int SQL_SERVER_2005_DEFAULT_DATETIME_SCALE = 3;
const int SQL_SERVER_2008_DEFAULT_DATETIME_PRECISION = 34;
const int SQL_SERVER_2008_DEFAULT_DATETIME_SCALE = 7;   

namespace AzureADOptions {
    const char AZURE_AUTH_SQL_PASSWORD[] = "SqlPassword";
    const char AZURE_AUTH_AD_PASSWORD[] = "ActiveDirectoryPassword";
}

// the message returned by ODBC Driver for SQL Server
const char ODBC_CONNECTION_BUSY_ERROR[] = "Connection is busy with results for another command";

// types for conversions on output parameters (though they can be used for input parameters, they are ignored)
enum HDB_PHPTYPE {
    MIN_HDB_PHPTYPE = 1, // lowest value for a php type
    HDB_PHPTYPE_NULL = 1,
    HDB_PHPTYPE_INT,
    HDB_PHPTYPE_FLOAT,
    HDB_PHPTYPE_STRING,
    HDB_PHPTYPE_DATETIME,
    HDB_PHPTYPE_STREAM,
    MAX_HDB_PHPTYPE,      // highest value for a php type
    HDB_PHPTYPE_INVALID = MAX_HDB_PHPTYPE     // used to see if a type is invalid
};

// encodings supported by this extension.  These basically translate into the use of SQL_C_CHAR or SQL_C_BINARY when getting
// information as a string or a stream.  
enum HDB_ENCODING {
    HDB_ENCODING_INVALID,        // unknown or invalid encoding.  Used to initialize variables.
    HDB_ENCODING_DEFAULT,        // use what is the connection's default for a statement, use system if a connection
    HDB_ENCODING_BINARY,         // use SQL_C_BINARY when using SQLGetData
    HDB_ENCODING_CHAR,           // use SQL_C_CHAR when using SQLGetData
    HDB_ENCODING_UTF8 = CP_UTF8,
    HDB_ENCODING_SYSTEM = HDB_ENCODING_UTF8,
};

// the array keys used when returning a row via hdb_fetch_array and hdb_fetch_object.
enum HDB_FETCH_TYPE {
    MIN_HDB_FETCH = 1,        // lowest value for fetch type
    HDB_FETCH_NUMERIC = 1,    // return an array with only numeric indices
    HDB_FETCH_ASSOC = 2,      // return an array with keys made from the field names
    HDB_FETCH_BOTH = 3,       // return an array indexed with both numbers and keys
    MAX_HDB_FETCH = 3,        // highest value for fetch type
};

// buffer size of a sql state (including the null character)
const int SQL_SQLSTATE_BUFSIZE = SQL_SQLSTATE_SIZE + 1;

// buffer size allocated to retrieve data from a PHP stream.  This number
// was chosen since PHP doesn't return more than 8k at a time even if
// the amount requested was more.
const int PHP_STREAM_BUFFER_SIZE = 8192;

// SQL types for parameters encoded in an integer.  The type corresponds to the SQL type ODBC constants.
// The size is the column size or precision, and scale is the decimal digits for precise numeric types.

union hdb_sqltype {
    struct typeinfo_t {
        int type:9;
        int size:14;
        int scale:8;
    } typeinfo;

    zend_long value;
};


// HDB PHP types (as opposed to the Zend PHP type constants).  Contains the type (see HDB_PHPTYPE)
// and the encoding for strings and streams (see HDB_ENCODING) 

union hdb_phptype {

    struct typeinfo_t {
        unsigned type:8;
        unsigned encoding:16;
    } typeinfo;

    zend_long value;
};

// static assert for enforcing compile time conditions
template <bool b>
struct hdb_static_assert;

template <>
struct hdb_static_assert<true> { _In_ static const int value = 1; };

#define HDB_STATIC_ASSERT( c )   (hdb_static_assert<(c) != 0>() )


//*********************************************************************************************************************************
// Logging
//*********************************************************************************************************************************
// log_callback
// a driver specific callback for logging messages
// severity - severity of the message: notice, warning, or error
// msg - the message to log in a FormatMessage style formatting
// print_args - args to the message
typedef void (*log_callback)( _In_ unsigned int severity, _In_ const char* msg, _In_opt_ va_list* print_args );

// each driver must register a log callback.  This should be the first thing a driver does.
void core_hdb_register_logger( _In_ log_callback );

// a simple wrapper around a PHP error logging function.
void write_to_log( _In_ unsigned int severity, _In_ const char* msg, ... );

// a macro to make it convenient to use the function.
#define LOG( severity, msg, ...)    write_to_log( severity, msg, ## __VA_ARGS__ )

// mask for filtering which severities are written to the log
enum logging_severity {
    SEV_ERROR = 0x01,
    SEV_WARNING = 0x02,
    SEV_NOTICE = 0x04,
    SEV_ALL = -1,
};

// Kill the PHP process and log the message to PHP
void die( _In_opt_ const char* msg, ... );
#define DIE( msg, ... ) { die( msg, ## __VA_ARGS__ ); }


//*********************************************************************************************************************************
// Resource/Memory Management
//*********************************************************************************************************************************

// the macro max is defined and overrides the call to max in the allocator class
#pragma push_macro( "max" )
#undef max

// new memory allocation/free debugging facilities to help us verify that all allocations are being 
// released in a timely manner and not just at the end of the script.  
// Zend has memory logging and checking, but it can generate a lot of noise for just one extension.
// It's meant for internal use but might be useful for people adding features to our extension.
// To use it, uncomment the #define below and compile in Debug NTS.  All allocations and releases
// must be done with hdb_malloc and hdb_free.
// #define HDB_MEM_DEBUG  1
#if defined( PHP_DEBUG ) && !defined( ZTS ) && defined( HDB_MEM_DEBUG )

inline void* hdb_malloc_trace( _In_ size_t size, _In_ const char* file, _In_ int line )
{
    void* ptr = emalloc( size );
    LOG( SEV_NOTICE, "emalloc returned %4!08x!: %1!d! bytes at %2!s!:%3!d!", size, file, line, ptr );
    return ptr;
}

inline void* hdb_malloc_trace( _In_ size_t element_count, _In_ size_t element_size, _In_ size_t extra, _In_ const char* file, _In_ int line )
{
    OACR_WARNING_SUPPRESS( ALLOC_SIZE_OVERFLOW_IN_ALLOC_WRAPPER, "Overflow verified below" );

    if(( element_count > 0 && element_size > 0 ) &&
        ( element_count > element_size * element_count || element_size > element_size * element_count )) {
          DIE( "Integer overflow in hdb_malloc" );
    }

    if( element_size * element_count > element_size * element_count + extra ) {
        DIE( "Integer overflow in hdb_malloc" );
    }

    if( element_size * element_count + extra == 0 ) {
        DIE( "Allocation size must be more than 0" );
    }

    void* ptr = emalloc( element_size * element_count + extra );
    LOG( SEV_NOTICE, "emalloc returned %4!08x!: %1!d! bytes at %2!s!:%3!d!", size, file, line, ptr );
    return ptr;
}

inline void* hdb_realloc_trace( void* buffer, _In_ size_t size, _In_ const char* file, _In_ int line )
{
    void* ptr = erealloc( original, size );
    LOG( SEV_NOTICE, "erealloc returned %5!08x! from %4!08x!: %1!d! bytes at %2!s!:%3!d!", size, file, line, ptr, original );
    return ptr;
}

inline void hdb_free_trace( _Inout_ void* ptr, _In_ const char* file, _In_ int line )
{
    LOG( SEV_NOTICE, "efree %1!08x! at %2!s!:%3!d!", ptr, file, line );
    efree( ptr );
}

#define hdb_malloc( size ) hdb_malloc_trace( size, __FILE__, __LINE__ )
#define hdb_malloc( count, size, extra ) hdb_malloc_trace( count, size, extra, __FILE__, __LINE__ )
#define hdb_realloc( buffer, size ) hdb_realloc_trace( buffer, size, __FILE__, __LINE__ )
#define hdb_free( ptr ) hdb_free_trace( ptr, __FILE__, __LINE__ )

#else

inline void* hdb_malloc( _In_ size_t size )
{
    return emalloc( size );
}

inline void* hdb_malloc( _In_ size_t element_count, _In_ size_t element_size, _In_ size_t extra )
{
    OACR_WARNING_SUPPRESS( ALLOC_SIZE_OVERFLOW_IN_ALLOC_WRAPPER, "Overflow verified below" );

    if(( element_count > 0 && element_size > 0 ) &&
        ( element_count > element_size * element_count || element_size > element_size * element_count )) {
          DIE( "Integer overflow in hdb_malloc" );
    }

    if( element_size * element_count > element_size * element_count + extra ) {
        DIE( "Integer overflow in hdb_malloc" );
    }

    if( element_size * element_count + extra == 0 ) {
        DIE( "Allocation size must be more than 0" );
    }

    return emalloc( element_size * element_count + extra );
}

inline void* hdb_realloc( _Inout_ void* buffer, _In_ size_t size )
{
    return erealloc( buffer, size );
}

inline void hdb_free( _Inout_ void* ptr )
{
    efree( ptr );
}

#endif

// trait class that allows us to assign const types to an auto_ptr
template <typename T>
struct remove_const {
    typedef T type;
};

template <typename T>
struct remove_const<const T*> {
    typedef T* type;
};

// allocator that uses the zend memory manager to manage memory
// this allows us to use STL classes that still work with Zend objects
template<typename T>
struct hdb_allocator {
   
    // typedefs used by the STL classes
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // conversion typedef (used by list and other STL classes)
    template<typename U>
    struct rebind {
        typedef hdb_allocator<U> other;
    };

    inline hdb_allocator() {}
    inline ~hdb_allocator() {}
    inline hdb_allocator( hdb_allocator const& ) {}
    template<typename U>
    inline hdb_allocator( hdb_allocator<U> const& ) {}

    // address (doesn't work if the class defines operator&)
    inline pointer address( _In_ reference r )
    { 
        return &r; 
    }

    inline const_pointer address( _In_ const_reference r )
    {
        return &r;
    }

    // memory allocation/deallocation
    inline pointer allocate( _In_ size_type cnt, 
                             typename std::allocator<void>::const_pointer = 0 )
    {
        return reinterpret_cast<pointer>( hdb_malloc(cnt, sizeof (T), 0)); 
    }

    inline void deallocate( _Inout_ pointer p, size_type ) 
    { 
        hdb_free(p); 
    }

    // size
    inline size_type max_size( void ) const 
    { 
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    // object construction/destruction
    inline void construct( _In_ pointer p, _In_ const T& t )
    {
        new(p) T(t);
    }

    inline void destroy( _Inout_ pointer p )
    {
        p->~T();
    }

    // equality operators
    inline bool operator==( hdb_allocator const& )
    {
        return true;
    }

    inline bool operator!=( _In_ hdb_allocator const& a )
    {
        return !operator==(a);
    }
}; 


// base class for auto_ptrs that we define below.  It provides common operators and functions 
// used by all the classes.  
template <typename T, typename Subclass>
class hdb_auto_ptr {

public:

    hdb_auto_ptr( void ) : _ptr( NULL )
    {
    }

    ~hdb_auto_ptr( void )
    {
        static_cast<Subclass*>(this)->reset( NULL );
    }

    // call when ownership is transferred
    void transferred( void )
    {
        _ptr = NULL;
    }

    // explicit function to get the pointer.
    T* get( void ) const
    {
        return _ptr;
    }

    // cast operator to allow auto_ptr to be used where a normal const * can be.
    operator const T* () const
    {
        return _ptr;
    }

    // cast operator to allow auto_ptr to be used where a normal pointer can be.
    operator typename remove_const<T*>::type () const
    {
        return _ptr;
    }

    operator bool() const
    {
        return _ptr != NULL;
    }

    // there are a number of places where we allocate a block intended to be accessed as
    // an array of elements, so this operator allows us to treat the memory as such.
    T& operator[]( _In_ int index ) const
    {
        return _ptr[ index ];
    }

    // there are a number of places where we allocate a block intended to be accessed as
    // an array of elements, so this operator allows us to treat the memory as such.
    T& operator[]( _In_ unsigned int index ) const
    {
        return _ptr[ index ];
    }

    // there are a number of places where we allocate a block intended to be accessed as
    // an array of elements, so this operator allows us to treat the memory as such.
    T& operator[]( _In_ long index ) const
    {
        return _ptr[ index ];
    }

	
	#ifdef __WIN64	
	// there are a number of places where we allocate a block intended to be accessed as
	// an array of elements, so this operator allows us to treat the memory as such.
	T& operator[]( _In_ std::size_t index ) const
	{
		return _ptr[index];
	}
	#endif

    // there are a number of places where we allocate a block intended to be accessed as
    // an array of elements, so this operator allows us to treat the memory as such.
    T& operator[]( _In_ unsigned short index ) const
    {
        return _ptr[ index ];
    }

    // access elements of a structure through the auto ptr
    T* const operator->( void ) const
    {
        return _ptr;
    }

    // value from reference operator (i.e., i = *(&i); or *i = blah;)
    T& operator*() const
    {
        return *_ptr;
    }

    // allow the use of the address-of operator to simulate a **.
    // Note: this operator conflicts with storing these within an STL container.  If you need
    // to do that, then redefine this as getpp and change instances of &auto_ptr to auto_ptr.getpp()
    T** operator&( void )
    {
        return &_ptr;
    }

protected:

    hdb_auto_ptr( _In_opt_ T* ptr ) :
        _ptr( ptr ) 
    {
    }

    hdb_auto_ptr( _Inout_opt_ hdb_auto_ptr& src )
    {
        if( _ptr ) {
            static_cast<Subclass*>(this)->reset( src._ptr );
        }
        src.transferred();
    }

    // assign a new pointer to the auto_ptr.  It will free the previous memory block
    // because ownership is deemed finished.
    T* operator=( _In_opt_ T* ptr )
    {
        static_cast<Subclass*>( this )->reset( ptr );

        return ptr;
    }

    T* _ptr;   
};

// an auto_ptr for hdb_malloc/hdb_free.  When allocating a chunk of memory using hdb_malloc, wrap that pointer
// in a variable of hdb_malloc_auto_ptr.  hdb_malloc_auto_ptr will "own" that block and assure that it is
// freed until the variable is destroyed (out of scope) or ownership is transferred using the function
// "transferred".
// DO NOT CALL hdb_realloc with a hdb_malloc_auto_ptr.  Use the resize member function.

template <typename T>
class hdb_malloc_auto_ptr : public hdb_auto_ptr<T, hdb_malloc_auto_ptr<T> > {

public:

    hdb_malloc_auto_ptr( void ) :
        hdb_auto_ptr<T, hdb_malloc_auto_ptr<T> >( NULL )
    {
    }

    hdb_malloc_auto_ptr( _Inout_opt_ const hdb_malloc_auto_ptr& src ) :
        hdb_auto_ptr<T, hdb_malloc_auto_ptr<T> >( src )
    {
    }

    // free the original pointer and assign a new pointer. Use NULL to simply free the pointer.
    void reset( _In_opt_ T* ptr = NULL )
    {
        if( hdb_auto_ptr<T,hdb_malloc_auto_ptr<T> >::_ptr )
            hdb_free( (void*) hdb_auto_ptr<T,hdb_malloc_auto_ptr<T> >::_ptr );
        hdb_auto_ptr<T,hdb_malloc_auto_ptr<T> >::_ptr = ptr;
    }

    T* operator=( _In_opt_ T* ptr )
    {
        return hdb_auto_ptr<T, hdb_malloc_auto_ptr<T> >::operator=( ptr );
    }

    void operator=( _Inout_opt_ hdb_malloc_auto_ptr<T>& src )
    {
        T* p = src.get();
        src.transferred();
        this->_ptr = p;
    }

    // DO NOT CALL hdb_realloc with a hdb_malloc_auto_ptr.  Use the resize member function.
    // has the same parameter list as hdb_realloc: new_size is the size in bytes of the newly allocated buffer
    void resize( _In_ size_t new_size )
    {	
    	hdb_auto_ptr<T,hdb_malloc_auto_ptr<T> >::_ptr = reinterpret_cast<T*>( hdb_realloc( hdb_auto_ptr<T,hdb_malloc_auto_ptr<T> >::_ptr, new_size ));
    }
};


// auto ptr for Zend hash tables.  Used to clean up a hash table allocated when 
// something caused an early exit from the function.  This is used when the hash_table is
// allocated in a zval that itself can't be released.  Otherwise, use the zval_auto_ptr.

class hash_auto_ptr : public hdb_auto_ptr<HashTable, hash_auto_ptr> {

public:

    hash_auto_ptr( void ) :
        hdb_auto_ptr<HashTable, hash_auto_ptr>( NULL )
    {
    }

    // free the original pointer and assign a new pointer. Use NULL to simply free the pointer.
    void reset( _In_opt_ HashTable* ptr = NULL )
    {
        if( _ptr ) {
            zend_hash_destroy( _ptr );
            FREE_HASHTABLE( _ptr );
        }
        _ptr = ptr;
    }

    HashTable* operator=( _In_opt_ HashTable* ptr )
    {
        return hdb_auto_ptr<HashTable, hash_auto_ptr>::operator=( ptr );
    }

private:

    hash_auto_ptr( _In_ HashTable const& hash );

    hash_auto_ptr( _In_ hash_auto_ptr const& hash );
};


// an auto_ptr for zvals.  When allocating a zval, wrap that pointer in a variable of zval_auto_ptr.  
// zval_auto_ptr will "own" that zval and assure that it is freed when the variable is destroyed 
// (out of scope) or ownership is transferred using the function "transferred".

class zval_auto_ptr : public hdb_auto_ptr<zval, zval_auto_ptr> {

public:

    zval_auto_ptr( void )
    {
    }

    // free the original pointer and assign a new pointer. Use NULL to simply free the pointer.
    void reset( _In_opt_ zval* ptr = NULL )
    {
        if( _ptr )
            zval_ptr_dtor(_ptr );
        _ptr = ptr;
    }

    zval* operator=( _In_opt_ zval* ptr )
    {
        return hdb_auto_ptr<zval, zval_auto_ptr>::operator=( ptr );
    }


private:

    zval_auto_ptr( _In_ const zval_auto_ptr& src );
};

#pragma pop_macro( "max" )


//*********************************************************************************************************************************
// hdb_error
//*********************************************************************************************************************************

// *** PHP specific errors ***
// hdb errors are held in a structure of this type used by the driver handle_error functions
// format is a flag that tells the driver error handler functions if there are parameters to use with FormatMessage
// into the error message before returning it.

// base class which can be instatiated with aggregates (see error constants)
struct hdb_error_const {

    SQLCHAR* sqlstate;
    SQLCHAR* native_message;
    SQLINTEGER native_code;
    bool format;
};

// subclass which is used by the core layer to instantiate ODBC errors
struct hdb_error : public hdb_error_const {

    hdb_error( void )
    {
        sqlstate = NULL;
        native_message = NULL;
        native_code = -1;
        format = false;
    }

    hdb_error( _In_ SQLCHAR* sql_state, _In_ SQLCHAR* message, _In_ SQLINTEGER code, _In_ bool printf_format = false )
    {
        sqlstate = reinterpret_cast<SQLCHAR*>( hdb_malloc( SQL_SQLSTATE_BUFSIZE ));
        native_message = reinterpret_cast<SQLCHAR*>( hdb_malloc( SQL_MAX_ERROR_MESSAGE_LENGTH + 1 ));
        strcpy_s( reinterpret_cast<char*>( sqlstate ), SQL_SQLSTATE_BUFSIZE, reinterpret_cast<const char*>( sql_state ));
        strcpy_s( reinterpret_cast<char*>( native_message ), SQL_MAX_ERROR_MESSAGE_LENGTH + 1, reinterpret_cast<const char*>( message ));
        native_code = code;
        format = printf_format;
    }
    
    hdb_error( _In_ hdb_error_const const& prototype )
    {
        hdb_error( prototype.sqlstate, prototype.native_message, prototype.native_code, prototype.format );
    }

    ~hdb_error( void )
    {
        if( sqlstate != NULL ) {
            hdb_free( sqlstate );
        }
        if( native_message != NULL ) {
            hdb_free( native_message );
        }
    }
};


// an auto_ptr for hdb_errors.  These call the destructor explicitly rather than call delete
class hdb_error_auto_ptr : public hdb_auto_ptr<hdb_error, hdb_error_auto_ptr > {

public:

    hdb_error_auto_ptr( void ) :
        hdb_auto_ptr<hdb_error, hdb_error_auto_ptr >( NULL )
    {
    }

    hdb_error_auto_ptr( _Inout_opt_ hdb_error_auto_ptr const& src ) :
        hdb_auto_ptr<hdb_error, hdb_error_auto_ptr >( (hdb_error_auto_ptr&) src )
    {
    }

    // free the original pointer and assign a new pointer. Use NULL to simply free the pointer.
    void reset( _In_opt_ hdb_error* ptr = NULL )
    {
        if( _ptr ) {
            _ptr->~hdb_error();
            hdb_free( (void*) _ptr );
        }
        _ptr = ptr;
    }

    hdb_error* operator=( _In_opt_ hdb_error* ptr )
    {
        return hdb_auto_ptr<hdb_error, hdb_error_auto_ptr >::operator=( ptr );
    }

    // unlike traditional assignment operators, the chained assignment of an auto_ptr doesn't make much
    // sense.  Only the last one would have anything in it.
    void operator=( _Inout_opt_ hdb_error_auto_ptr& src )
    {
        hdb_error* p = src.get();
        src.transferred();
        reset( p );
    }
};


//*********************************************************************************************************************************
// Context
//*********************************************************************************************************************************

class hdb_context;
struct hdb_conn;

// error_callback
// a driver specific callback for processing errors. 
// ctx - the context holding the handles 
// hdb_error_code - specific error code to return.
typedef bool (*error_callback)( _Inout_ hdb_context& ctx, _In_ unsigned int hdb_error_code, _In_ bool error, _In_opt_ va_list* print_args );

// hdb_context
// a context holds relevant information to be passed with a connection and statement objects.

class hdb_context {

 public:

    hdb_context( _In_opt_ SQLSMALLINT type, _In_ error_callback e, _In_opt_ void* drv, _In_ HDB_ENCODING encoding = HDB_ENCODING_INVALID ) :
        handle_( SQL_NULL_HANDLE ),
        handle_type_( type ),
        name_( NULL ),
        err_( e ),
        driver_( drv ),
        last_error_(),
        encoding_( encoding )
    {
    }

    hdb_context( _In_ SQLHANDLE h, _In_opt_ SQLSMALLINT t, _In_ error_callback e, _In_opt_ void* drv, _In_ HDB_ENCODING encoding = HDB_ENCODING_INVALID ) :
        handle_( h ),
        handle_type_( t ),
        name_( NULL ),
        err_( e ),
        driver_( drv ),
        last_error_(),
        encoding_( encoding )
    {
    }

    hdb_context( _In_ hdb_context const& ctx ) :
        handle_( ctx.handle_ ),
        handle_type_( ctx.handle_type_ ),
        name_( ctx.name_ ),
        err_( ctx.err_ ),
        driver_( ctx.driver_ ),
        last_error_( ctx.last_error_ )
    {
    }

    virtual ~hdb_context()
    {        
    }

    void set_func( _In_z_ const char* f )
    {
        name_ = f;
    }

    void set_last_error( _In_ hdb_error_auto_ptr& last_error )
    {
        last_error_ = last_error;
    }

    hdb_error_auto_ptr& last_error( void )
    {
        return last_error_;
    }
    
    // since the primary responsibility of a context is to hold an ODBC handle, we
    // provide these convenience operators for using them interchangeably
    operator SQLHANDLE ( void ) const
    {
        return handle_;
    }

    error_callback error_handler( void ) const
    {
        return err_;
    }

    SQLHANDLE handle( void ) const
    {
        return handle_;
    }

    SQLSMALLINT handle_type( void ) const
    {
        return handle_type_;
    }

    const char* func( void ) const
    {
        return name_;
    }

    void* driver( void ) const
    {
        return driver_;
    }

    void set_driver( _In_ void* driver )
    {
        this->driver_ = driver;
    }

    void invalidate( void )
    {
        if( handle_ != SQL_NULL_HANDLE ) {
            ::SQLFreeHandle( handle_type_, handle_ );

			last_error_.reset();
        }
        handle_ = SQL_NULL_HANDLE;
    }

    bool valid( void )
    {
        return handle_ != SQL_NULL_HANDLE;
    }

    HDB_ENCODING encoding( void ) const
    {
        return encoding_;
    }

    void set_encoding( _In_ HDB_ENCODING e )
    {
        encoding_ = e;
    }

 private:
    SQLHANDLE              handle_;          // ODBC handle for this context
    SQLSMALLINT            handle_type_;     // type of the ODBC handle
    const char*            name_;            // function name currently executing this context
    error_callback         err_;             // driver error callback if error occurs in core layer
    void*                  driver_;          // points back to the driver for PDO
    hdb_error_auto_ptr  last_error_;      // last error that happened on this object
    HDB_ENCODING        encoding_;        // encoding of the context   
};

const int HDB_OS_VISTA_OR_LATER = 6;           // major version for Vista

// maps an IANA encoding to a code page
struct hdb_encoding {

    const char* iana;
    size_t iana_len;
    unsigned int code_page;
    bool not_for_connection;

    hdb_encoding( _In_ const char* iana, _In_ unsigned int code_page, _In_ bool not_for_conn = false ):
        iana( iana ), iana_len( strnlen_s( iana )), code_page( code_page ), not_for_connection( not_for_conn )
    {
    }
};


//*********************************************************************************************************************************
// Initialization
//*********************************************************************************************************************************

// variables set during initialization
extern bool isVistaOrGreater;                     // used to determine if OS is Vista or Greater
extern HashTable* g_encodings;                    // encodings supported by this driver

void core_hdb_minit( _Outptr_ hdb_context** henv_cp, _Inout_ hdb_context** henv_ncp, _In_ error_callback err, _In_z_ const char* driver_func );
void core_hdb_mshutdown( _Inout_ hdb_context& henv_cp, _Inout_ hdb_context& henv_ncp );

// environment context used by hdb_connect for when a connection error occurs.
struct hdb_henv {

    hdb_context ctx;

    hdb_henv( _In_ SQLHANDLE handle, _In_ error_callback e, _In_opt_ void* drv  ) :
        ctx( handle, SQL_HANDLE_ENV, e, drv )
    {
    }
};

//*********************************************************************************************************************************
// Connection
//*********************************************************************************************************************************

// supported server versions (determined at connection time)
enum SERVER_VERSION {
    SERVER_VERSION_UNKNOWN = -1,
    SERVER_VERSION_2000 = 8,
    SERVER_VERSION_2005,
    SERVER_VERSION_2008, // use this for anything 2008 or later
};

// supported driver versions.
// the latest RTWed ODBC is the first one
enum DRIVER_VERSION {
    ODBC_DRIVER_UNKNOWN = -1,
    FIRST = 0,
    ODBC_DRIVER_17 = FIRST,
    ODBC_DRIVER_13 = 1,
    ODBC_DRIVER_11 = 2,
    LAST = ODBC_DRIVER_11
};

// forward decl
struct hdb_stmt;
struct stmt_option;

// This holds the various details of column encryption. 
struct col_encryption_option {
    bool            enabled;            // column encryption enabled, false by default
    SQLINTEGER      akv_mode;
    zval_auto_ptr   akv_id;
    zval_auto_ptr   akv_secret;
    bool            akv_required;

    col_encryption_option() : enabled( false ), akv_mode(-1), akv_required( false )
    {
    }
};

// *** connection resource structure ***
// this is the resource structure returned when a connection is made.
struct hdb_conn : public hdb_context {

    // instance variables
    SERVER_VERSION server_version;  // version of the server that we're connected to

    col_encryption_option ce_option;    // holds the details of what are required to enable column encryption
    DRIVER_VERSION driver_version;      // version of ODBC driver

    // initialize with default values
    hdb_conn( _In_ SQLHANDLE h, _In_ error_callback e, _In_opt_ void* drv, _In_ HDB_ENCODING encoding ) :
        hdb_context( h, SQL_HANDLE_DBC, e, drv, encoding )
    {
        server_version = SERVER_VERSION_UNKNOWN;
        driver_version = ODBC_DRIVER_UNKNOWN;
    }

    // hdb_conn has no destructor since its allocated using placement new, which requires that the destructor be 
    // called manually.  Instead, we leave it to the allocator to invalidate the handle when an error occurs allocating
    // the hdb_conn with a connection.
};

enum HDB_STMT_OPTIONS {
   
   HDB_STMT_OPTION_INVALID,
   HDB_STMT_OPTION_QUERY_TIMEOUT,
   HDB_STMT_OPTION_SEND_STREAMS_AT_EXEC,
   HDB_STMT_OPTION_SCROLLABLE,
   HDB_STMT_OPTION_CLIENT_BUFFER_MAX_SIZE,

   // Driver specific connection options
   HDB_STMT_OPTION_DRIVER_SPECIFIC = 1000,

};

namespace ODBCConnOptions {

const char APP[] = "APP";
const char ApplicationIntent[] = "ApplicationIntent";
const char AttachDBFileName[] = "AttachDbFileName";
const char Authentication[] = "Authentication";
const char Driver[] = "DRIVER";
const char CharacterSet[] = "CharacterSet";
const char ConnectionPooling[] = "ConnectionPooling";
const char ColumnEncryption[] = "ColumnEncryption";
const char ConnectRetryCount[] = "ConnectRetryCount";
const char ConnectRetryInterval[] = "ConnectRetryInterval";
const char Database[] = "Database";
const char Encrypt[] = "Encrypt";
const char Failover_Partner[] = "Failover_Partner";
const char KeyStoreAuthentication[] = "KeyStoreAuthentication";
const char KeyStorePrincipalId[] = "KeyStorePrincipalId";
const char KeyStoreSecret[] = "KeyStoreSecret";
const char LoginTimeout[] = "LoginTimeout";
const char MARS_ODBC[] = "MARS_Connection";
const char MultiSubnetFailover[] = "MultiSubnetFailover";
const char QuotedId[] = "QuotedId";
const char TraceFile[] = "TraceFile";
const char TraceOn[] = "TraceOn";
const char TrustServerCertificate[] = "TrustServerCertificate";
const char TransactionIsolation[] = "TransactionIsolation";
const char TransparentNetworkIPResolution[] = "TransparentNetworkIPResolution";
const char WSID[] = "WSID";
const char UID[] = "UID";
const char PWD[] = "PWD";
const char SERVER[] = "SERVERNODE";

}

enum HDB_CONN_OPTIONS {
   
    HDB_CONN_OPTION_INVALID,
    HDB_CONN_OPTION_APP,
    HDB_CONN_OPTION_CHARACTERSET,
    HDB_CONN_OPTION_CONN_POOLING,
    HDB_CONN_OPTION_DATABASE,
    HDB_CONN_OPTION_ENCRYPT,
    HDB_CONN_OPTION_FAILOVER_PARTNER,
    HDB_CONN_OPTION_LOGIN_TIMEOUT,
    HDB_CONN_OPTION_MARS,
    HDB_CONN_OPTION_QUOTED_ID,
    HDB_CONN_OPTION_TRACE_FILE,
    HDB_CONN_OPTION_TRACE_ON,
    HDB_CONN_OPTION_TRANS_ISOLATION,
    HDB_CONN_OPTION_TRUST_SERVER_CERT,
    HDB_CONN_OPTION_WSID,
    HDB_CONN_OPTION_ATTACHDBFILENAME,
    HDB_CONN_OPTION_APPLICATION_INTENT,
    HDB_CONN_OPTION_MULTI_SUBNET_FAILOVER,
    HDB_CONN_OPTION_AUTHENTICATION,
    HDB_CONN_OPTION_COLUMNENCRYPTION,
    HDB_CONN_OPTION_DRIVER,
    HDB_CONN_OPTION_CEKEYSTORE_PROVIDER,
    HDB_CONN_OPTION_CEKEYSTORE_NAME,
    HDB_CONN_OPTION_CEKEYSTORE_ENCRYPT_KEY,
    HDB_CONN_OPTION_KEYSTORE_AUTHENTICATION,
    HDB_CONN_OPTION_KEYSTORE_PRINCIPAL_ID,
    HDB_CONN_OPTION_KEYSTORE_SECRET,
    HDB_CONN_OPTION_TRANSPARENT_NETWORK_IP_RESOLUTION,
    HDB_CONN_OPTION_CONN_RETRY_COUNT,
    HDB_CONN_OPTION_CONN_RETRY_INTERVAL,

   // Driver specific connection options
   HDB_CONN_OPTION_DRIVER_SPECIFIC = 1000,
   
};


#define NO_ATTRIBUTE -1

// type of connection attributes
enum CONN_ATTR_TYPE {
    CONN_ATTR_INT,
    CONN_ATTR_BOOL,
    CONN_ATTR_STRING,
    CONN_ATTR_INVALID,
};

// a connection option that includes the callback function that handles that option (e.g., adds it to the connection string or
// sets an attribute)
struct connection_option {
    // the name of the option as passed in by the user
    const char *        hdb_name;
    unsigned int        hdb_len;
    
    unsigned int        conn_option_key;
    // the name of the option in the ODBC connection string
    const char *        odbc_name;
    unsigned int        odbc_len;
    enum CONN_ATTR_TYPE value_type;

    // process the connection type
    // return whether or not the function was successful in processing the connection option
    void                (*func)( connection_option const*, zval* value, hdb_conn* conn, std::string& conn_str );
};

// connection attribute functions

// simply add the parsed value to the connection string
struct conn_str_append_func {
    static void func( _In_ connection_option const* option, _In_ zval* value, hdb_conn* /*conn*/, _Inout_ std::string& conn_str );
};

struct conn_null_func {
    static void func( connection_option const* /*option*/, zval* /*value*/, hdb_conn* /*conn*/, std::string& /*conn_str*/ );
};

struct column_encryption_set_func {
    static void func( _In_ connection_option const* option, _In_ zval* value, _Inout_ hdb_conn* conn, _Inout_ std::string& conn_str );
};

struct driver_set_func {   
    static void func( _In_ connection_option const* option, _In_ zval* value, _Inout_ hdb_conn* conn, _Inout_ std::string& conn_str );
};

struct ce_ksp_provider_set_func {
    static void func( _In_ connection_option const* option, _In_ zval* value, _Inout_ hdb_conn* conn, _Inout_ std::string& conn_str );
};

struct ce_akv_str_set_func {
   static void func( _In_ connection_option const* option, _In_ zval* value, _Inout_ hdb_conn* conn, _Inout_ std::string& conn_str );
};


// factory to create a connection (since they are subclassed to instantiate statements)
typedef hdb_conn* (*driver_conn_factory)( _In_ SQLHANDLE h, _In_ error_callback e, _In_ void* drv );

// *** connection functions ***
hdb_conn* core_hdb_connect( _In_ hdb_context& henv_cp, _In_ hdb_context& henv_ncp, _In_ driver_conn_factory conn_factory,
                                  _Inout_z_ const char* server, _Inout_opt_z_ const char* uid, _Inout_opt_z_ const char* pwd, 
                                  _Inout_opt_ HashTable* options_ht, _In_ error_callback err, _In_ const connection_option valid_conn_opts[], 
                                  _In_ void* driver, _In_z_ const char* driver_func );
SQLRETURN core_odbc_connect( _Inout_ hdb_conn* conn, _Inout_ std::string& conn_str, _In_ bool is_pooled );
void core_hdb_close( _Inout_opt_ hdb_conn* conn );
void core_hdb_prepare( _Inout_ hdb_stmt* stmt, _In_reads_bytes_(sql_len) const char* sql, _In_ SQLLEN sql_len );
void core_hdb_begin_transaction( _Inout_ hdb_conn* conn );
void core_hdb_commit( _Inout_ hdb_conn* conn );
void core_hdb_rollback( _Inout_ hdb_conn* conn );
void core_hdb_get_server_info( _Inout_ hdb_conn* conn, _Out_ zval* server_info );
void core_hdb_get_server_version( _Inout_ hdb_conn* conn, _Inout_ zval *server_version );
void core_hdb_get_client_info( _Inout_ hdb_conn* conn, _Out_ zval *client_info );
bool core_is_conn_opt_value_escaped( _Inout_ const char* value, _Inout_ size_t value_len );
size_t core_str_zval_is_true( _Inout_ zval* str_zval );
bool core_is_authentication_option_valid( _In_z_ const char* value, _In_ size_t value_len );
bool core_search_odbc_driver_unix( _In_ DRIVER_VERSION driver_version );
bool core_compare_error_state( _In_ hdb_conn* conn,  _In_ SQLRETURN r, _In_ const char* error_state );

//*********************************************************************************************************************************
// Statement
//*********************************************************************************************************************************

struct stmt_option_functor {

    virtual void operator()( _Inout_ hdb_stmt* /*stmt*/, stmt_option const* /*opt*/, _In_ zval* /*value_z*/ );
};

struct stmt_option_query_timeout : public stmt_option_functor {

    virtual void operator()( _Inout_ hdb_stmt* stmt, stmt_option const* opt, _In_ zval* value_z );
};

struct stmt_option_send_at_exec : public stmt_option_functor {

    virtual void operator()( _Inout_ hdb_stmt* stmt, stmt_option const* opt, _In_ zval* value_z );
};

struct stmt_option_buffered_query_limit : public stmt_option_functor {

    virtual void operator()( _Inout_ hdb_stmt* stmt, stmt_option const* opt, _In_ zval* value_z );
};

// used to hold the table for statment options
struct stmt_option {

    const char *         name;        // name of the statement option
    unsigned int         name_len;    // name length
    unsigned int         key;         
    std::unique_ptr<stmt_option_functor> func;        // callback that actually handles the work of the option
    
};

// holds the stream param and the encoding that it was assigned
struct hdb_stream {

    zval* stream_z;
    HDB_ENCODING encoding;
    SQLUSMALLINT field_index;
    SQLSMALLINT sql_type;
    hdb_stmt* stmt;

    hdb_stream( _In_opt_ zval* str_z, _In_ HDB_ENCODING enc ) :
        stream_z( str_z ), encoding( enc ), field_index( 0 ), sql_type( SQL_UNKNOWN_TYPE ), stmt( NULL )
    {
    }

    hdb_stream() : stream_z( NULL ), encoding( HDB_ENCODING_INVALID ), field_index( 0 ), sql_type( SQL_UNKNOWN_TYPE ), stmt( NULL )
    {
    }
};

// close any active stream
void close_active_stream( _Inout_ hdb_stmt* stmt );

extern php_stream_wrapper g_hdb_stream_wrapper;

// resource constants used when registering the stream type with PHP
#define HDB_STREAM_WRAPPER "hdb"
#define HDB_STREAM         "hdb_stream"

// holds the output parameter information.  Strings also need the encoding and other information for
// after processing.  Only integer, float, and strings are allowable output parameters.
struct hdb_output_param {

    zval* param_z;
    HDB_ENCODING encoding;
    SQLUSMALLINT param_num;         // used to index into the ind_or_len of the statement
    SQLLEN original_buffer_len;     // used to make sure the returned length didn't overflow the buffer
    HDB_PHPTYPE php_out_type;    // used to convert output param if necessary
    bool is_bool;

    // string output param constructor
    hdb_output_param( _In_ zval* p_z, _In_ HDB_ENCODING enc, _In_ int num, _In_ SQLUINTEGER buffer_len ) :
        param_z(p_z), encoding(enc), param_num(num), original_buffer_len(buffer_len), is_bool(false), php_out_type(HDB_PHPTYPE_INVALID)
    {
    }

    // every other type output parameter constructor
    hdb_output_param( _In_ zval* p_z, _In_ int num, _In_ bool is_bool, _In_ HDB_PHPTYPE php_out_type) :
        param_z( p_z ),
        encoding( HDB_ENCODING_INVALID ),
        param_num( num ),
        original_buffer_len( -1 ),
        is_bool( is_bool ),
        php_out_type(php_out_type)
    {
    }
};

// forward decls
struct hdb_result_set;

// *** parameter metadata struct ***
struct param_meta_data
{
    SQLSMALLINT sql_type; 
    SQLSMALLINT decimal_digits;
    SQLSMALLINT nullable; 
    SQLULEN     column_size;

    param_meta_data() : sql_type(0), decimal_digits(0), column_size(0), nullable(0)
    {
    }

    ~param_meta_data()
    {
    }

    SQLSMALLINT get_sql_type() { return sql_type; }
    SQLSMALLINT get_decimal_digits() { return decimal_digits; }
    SQLSMALLINT get_nullable() { return nullable; }
    SQLULEN get_column_size() { return column_size; }
};

// *** Statement resource structure *** 
struct hdb_stmt : public hdb_context {

    void free_param_data( );
    virtual void new_result_set( );

    hdb_conn*   conn;                  // Connection that created this statement
   
    bool executed;                        // Whether the statement has been executed yet (used for error messages)
    bool past_fetch_end;                  // Core_hdb_fetch sets this field when the statement goes beyond the last row
    hdb_result_set* current_results;   // Current result set
    SQLULEN cursor_type;                  // Type of cursor for the current result set
    bool has_rows;                        // Has_rows is set if there are actual rows in the row set
    bool fetch_called;                    // Used by core_hdb_get_field to return an informative error if fetch not yet called 
    int last_field_index;                 // last field retrieved by core_hdb_get_field
    bool past_next_result_end;            // core_hdb_next_result sets this to true when the statement goes beyond the 
                                          // last results
    unsigned long query_timeout;          // maximum allowed statement execution time
    zend_long buffered_query_limit;       // maximum allowed memory for a buffered query (measured in KB)

    // holds output pointers for SQLBindParameter
    // We use a deque because it 1) provides the at/[] access in constant time, and 2) grows dynamically without moving
    // memory, which is important because we pass the pointer to an element of the deque to SQLBindParameter to hold
    std::deque<SQLLEN>   param_ind_ptrs;  // output pointers for lengths for calls to SQLBindParameter
    zval param_input_strings;             // hold all UTF-16 input strings that aren't managed by PHP
    zval output_params;                   // hold all the output parameters
    zval param_streams;                   // track which streams to send data to the server
    zval param_datetime_buffers;          // datetime strings to be converted back to DateTime objects
    bool send_streams_at_exec;            // send all stream data right after execution before returning
    hdb_stream current_stream;         // current stream sending data to the server as an input parameter
    unsigned int current_stream_read;     // # of bytes read so far. (if we read an empty PHP stream, we send an empty string 
                                          // to the server)
    zval field_cache;                     // cache for a single row of fields, to allow multiple and out of order retrievals
    zval col_cache;                       // Used by get_field_as_string not to call SQLColAttribute()  after every fetch. 
    zval active_stream;                   // the currently active stream reading data from the database

    std::vector<param_meta_data> param_descriptions;

    hdb_stmt( _In_ hdb_conn* c, _In_ SQLHANDLE handle, _In_ error_callback e, _In_opt_ void* drv );
    virtual ~hdb_stmt( void );

    // driver specific conversion rules from a SQL Server/ODBC type to one of the HDB_PHPTYPE_* constants
    virtual hdb_phptype sql_type_to_php_type( _In_ SQLINTEGER sql_type, _In_ SQLUINTEGER size, _In_ bool prefer_string_to_stream ) = 0;

};

// *** field metadata struct ***
struct field_meta_data {

    hdb_malloc_auto_ptr<SQLCHAR> field_name;
    SQLSMALLINT field_name_len;
    SQLSMALLINT field_type;
    SQLULEN field_size;
    SQLULEN field_precision;
    SQLSMALLINT field_scale;     
    SQLSMALLINT field_is_nullable;

    field_meta_data() : field_name_len(0), field_type(0), field_size(0), field_precision(0),
                        field_scale (0), field_is_nullable(0)
    {
    }

    ~field_meta_data() 
    {
    }
};

// *** statement constants ***
// unknown column size used by core_hdb_bind_param when the user doesn't supply a value
const SQLULEN HDB_UNKNOWN_SIZE = 0xffffffff;
const int HDB_DEFAULT_SIZE = -1;     // size given for an output parameter that doesn't really need one (e.g., int)

// uninitialized query timeout value
const unsigned int QUERY_TIMEOUT_INVALID = 0xffffffff;

// special buffered query constant
#ifndef _WIN32
const size_t HDB_CURSOR_BUFFERED = 42; // arbitrary number that doesn't map to any other SQL_CURSOR_* constant
#else
const size_t HDB_CURSOR_BUFFERED = 0xfffffffeUL; // arbitrary number that doesn't map to any other SQL_CURSOR_* constant
#endif // !_WIN32

// factory to create a statement
typedef hdb_stmt* (*driver_stmt_factory)( hdb_conn* conn, SQLHANDLE h, error_callback e, void* drv );

// *** statement functions ***
hdb_stmt* core_hdb_create_stmt( _Inout_ hdb_conn* conn, _In_ driver_stmt_factory stmt_factory, _In_opt_ HashTable* options_ht, 
                                      _In_opt_ const stmt_option valid_stmt_opts[], _In_ error_callback const err, _In_opt_ void* driver );
void core_hdb_bind_param( _Inout_ hdb_stmt* stmt, _In_ SQLUSMALLINT param_num, _In_ SQLSMALLINT direction, _Inout_ zval* param_z,
                             _In_ HDB_PHPTYPE php_out_type, _Inout_ HDB_ENCODING encoding, _Inout_ SQLSMALLINT sql_type, _Inout_ SQLULEN column_size,
                             _Inout_ SQLSMALLINT decimal_digits );
SQLRETURN core_hdb_execute( _Inout_ hdb_stmt* stmt, _In_reads_bytes_(sql_len) const char* sql = NULL, _In_ int sql_len = 0 );
field_meta_data* core_hdb_field_metadata( _Inout_ hdb_stmt* stmt, _In_ SQLSMALLINT colno );
bool core_hdb_fetch( _Inout_ hdb_stmt* stmt, _In_ SQLSMALLINT fetch_orientation, _In_ SQLULEN fetch_offset );
void core_hdb_get_field( _Inout_ hdb_stmt* stmt, _In_ SQLUSMALLINT field_index, _In_ hdb_phptype hdb_phptype, _In_ bool prefer_string,
							_Outref_result_bytebuffer_maybenull_(*field_length) void*& field_value, _Inout_ SQLLEN* field_length, _In_ bool cache_field,
							_Out_ HDB_PHPTYPE *hdb_php_type_out);
bool core_hdb_has_any_result( _Inout_ hdb_stmt* stmt );
void core_hdb_next_result( _Inout_ hdb_stmt* stmt, _In_ bool finalize_output_params = true, _In_ bool throw_on_errors = true );
void core_hdb_post_param( _Inout_ hdb_stmt* stmt, _In_ zend_ulong paramno, zval* param_z );
void core_hdb_set_scrollable( _Inout_ hdb_stmt* stmt, _In_ unsigned long cursor_type );
void core_hdb_set_query_timeout( _Inout_ hdb_stmt* stmt, _In_ long timeout );
void core_hdb_set_query_timeout( _Inout_ hdb_stmt* stmt, _Inout_ zval* value_z );
void core_hdb_set_send_at_exec( _Inout_ hdb_stmt* stmt, _In_ zval* value_z );
bool core_hdb_send_stream_packet( _Inout_ hdb_stmt* stmt );
void core_hdb_set_buffered_query_limit( _Inout_ hdb_stmt* stmt, _In_ zval* value_z );
void core_hdb_set_buffered_query_limit( _Inout_ hdb_stmt* stmt, _In_ SQLLEN limit );


//*********************************************************************************************************************************
// Result Set
//*********************************************************************************************************************************

// Abstract the result set so that a result set can either be used as is from ODBC or buffered.
// This is not a complete abstraction of a result set.  Only enough is abstracted to allow for
// information and capabilities normally not available when a result set is not buffered
// (e.g., forward only vs buffered means row count is available and cursor movement is possible).
// Otherwise, normal ODBC calls are still valid and should be used to get information about the
// result set (e.g., SQLNumResultCols).

struct hdb_result_set {

    hdb_stmt* odbc;

    explicit hdb_result_set( _In_ hdb_stmt* );
    virtual ~hdb_result_set( void ) { }

    virtual bool cached( int field_index ) = 0;
    virtual SQLRETURN fetch( _Inout_ SQLSMALLINT fetch_orientation, _Inout_opt_ SQLLEN fetch_offset ) = 0;
    virtual SQLRETURN get_data( _In_ SQLUSMALLINT field_index, _In_ SQLSMALLINT target_type,
                                _Out_writes_bytes_opt_(buffer_length) void* buffer, _In_ SQLLEN buffer_length, _Inout_ SQLLEN* out_buffer_length,
                                bool handle_warning )= 0;
    virtual SQLRETURN get_diag_field( _In_ SQLSMALLINT record_number, _In_ SQLSMALLINT diag_identifier, 
                                      _Inout_updates_(buffer_length) SQLPOINTER diag_info_buffer, _In_ SQLSMALLINT buffer_length,
                                      _Inout_ SQLSMALLINT* out_buffer_length ) = 0;
    virtual hdb_error* get_diag_rec( _In_ SQLSMALLINT record_number ) = 0;
    virtual SQLLEN row_count( ) = 0;
};

struct hdb_odbc_result_set : public hdb_result_set {

    explicit hdb_odbc_result_set( _In_ hdb_stmt* );
	virtual ~hdb_odbc_result_set( void );

    virtual bool cached( int field_index ) { return false; }
    virtual SQLRETURN fetch( _In_ SQLSMALLINT fetch_orientation, _In_ SQLLEN fetch_offset );
    virtual SQLRETURN get_data( _In_ SQLUSMALLINT field_index, _In_ SQLSMALLINT target_type,
                                _Out_writes_opt_(buffer_length) void* buffer, _In_ SQLLEN buffer_length, _Inout_ SQLLEN* out_buffer_length,
                                _In_ bool handle_warning );
    virtual SQLRETURN get_diag_field( _In_ SQLSMALLINT record_number, _In_ SQLSMALLINT diag_identifier, 
                                      _Inout_updates_(buffer_length) SQLPOINTER diag_info_buffer, _In_ SQLSMALLINT buffer_length,
                                      _Inout_ SQLSMALLINT* out_buffer_length );
    virtual hdb_error* get_diag_rec( _In_ SQLSMALLINT record_number );
    virtual SQLLEN row_count( );

 private:
    // prevent invalid instantiations and assignments
    hdb_odbc_result_set( void );
    hdb_odbc_result_set( hdb_odbc_result_set& );
    hdb_odbc_result_set& operator=( hdb_odbc_result_set& );
};

struct hdb_buffered_result_set : public hdb_result_set {

    struct meta_data {
        SQLSMALLINT type;
        SQLSMALLINT c_type;     // convenience
        SQLULEN offset;         // in bytes
        SQLULEN length;         // in bytes
        SQLSMALLINT scale;

        static const SQLULEN SIZE_UNKNOWN = 0;
    };

    // default maximum amount of memory that a buffered query can consume
    #define INI_BUFFERED_QUERY_LIMIT_DEFAULT    "10240" // default used by the php.ini settings
    static const zend_long BUFFERED_QUERY_LIMIT_DEFAULT = 10240;   // measured in KB
    static const zend_long BUFFERED_QUERY_LIMIT_INVALID = 0;

    explicit hdb_buffered_result_set( _Inout_ hdb_stmt* odbc );
    virtual ~hdb_buffered_result_set( void );

    virtual bool cached( int field_index ) { return true; }
    virtual SQLRETURN fetch( _Inout_ SQLSMALLINT fetch_orientation, _Inout_opt_ SQLLEN fetch_offset );
    virtual SQLRETURN get_data( _In_ SQLUSMALLINT field_index, _In_ SQLSMALLINT target_type,
                                _Out_writes_bytes_opt_(buffer_length) void* buffer, _In_ SQLLEN buffer_length, _Inout_ SQLLEN* out_buffer_length,
                                bool handle_warning );
    virtual SQLRETURN get_diag_field( _In_ SQLSMALLINT record_number, _In_ SQLSMALLINT diag_identifier, 
                                      _Inout_updates_(buffer_length) SQLPOINTER diag_info_buffer, _In_ SQLSMALLINT buffer_length,
                                      _Inout_ SQLSMALLINT* out_buffer_length );
    virtual hdb_error* get_diag_rec( _In_ SQLSMALLINT record_number );
    virtual SQLLEN row_count( );

    // buffered result set specific 
    SQLSMALLINT column_count( void )
    {
        return col_count;
    }

    struct meta_data& col_meta_data( SQLSMALLINT i )
    {
        return meta[i];
    }

 private:
    // prevent invalid instantiations and assignments
    hdb_buffered_result_set( void );
    hdb_buffered_result_set( hdb_buffered_result_set& );
    hdb_buffered_result_set& operator=( hdb_buffered_result_set& );

    HashTable* cache;                   // rows of data kept in index based hash table
    SQLSMALLINT col_count;            // number of columns in the current result set
    hdb_malloc_auto_ptr<meta_data> meta;  // metadata for fields in the cache
    SQLLEN current;                     // 1 based, 0 means before first row
    hdb_error_auto_ptr last_error;   // if an error occurred, it is kept here
    SQLUSMALLINT last_field_index;      // the last field data retrieved from
    SQLLEN read_so_far;                 // position within string to read from (for partial reads of strings)
    hdb_malloc_auto_ptr<SQLCHAR> temp_string;   // temp buffer to hold a converted field while in use
    SQLLEN temp_length;                 // number of bytes in the temp conversion buffer

    typedef SQLRETURN (hdb_buffered_result_set::*conv_fn)( _In_ SQLSMALLINT field_index, _Out_writes_z_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                                              _Inout_ SQLLEN* out_buffer_length );
    typedef std::map< SQLINTEGER, std::map< SQLINTEGER, conv_fn > > conv_matrix_t;

    // two dimentional sparse matrix that holds the [from][to] functions that do conversions
    static conv_matrix_t conv_matrix;

    // string conversion functions
    SQLRETURN binary_to_wide_string( _In_ SQLSMALLINT field_index, _Out_writes_z_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                     _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN binary_to_system_string( _In_ SQLSMALLINT field_index, _Out_writes_z_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length, 
                                       _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN system_to_wide_string( _In_ SQLSMALLINT field_index, _Out_writes_z_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                     _Out_ SQLLEN* out_buffer_length );
    SQLRETURN to_binary_string( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                _Out_ SQLLEN* out_buffer_length );
    SQLRETURN to_same_string( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                               _Out_ SQLLEN* out_buffer_length );
    SQLRETURN wide_to_system_string( _In_ SQLSMALLINT field_index, _Inout_updates_bytes_to_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                     _Inout_ SQLLEN* out_buffer_length );

    // long conversion functions
    SQLRETURN to_long( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length, _Out_ SQLLEN* out_buffer_length );
    SQLRETURN long_to_system_string( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                     _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN long_to_wide_string( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                     _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN long_to_double( _In_ SQLSMALLINT field_index, _Out_writes_bytes_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                              _Out_ SQLLEN* out_buffer_length );

    // double conversion functions
    SQLRETURN to_double( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length, _Out_ SQLLEN* out_buffer_length );
    SQLRETURN double_to_system_string( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                       _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN double_to_wide_string( _In_ SQLSMALLINT field_index, _Out_writes_bytes_to_opt_(buffer_length, *out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                     _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN double_to_long( _In_ SQLSMALLINT field_index, _Inout_updates_bytes_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                              _Inout_ SQLLEN* out_buffer_length );

    // string to number conversion functions
    // Future: See if these can be converted directly to template member functions
    SQLRETURN string_to_double( _In_ SQLSMALLINT field_index, _Out_writes_bytes_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                 _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN string_to_long( _In_ SQLSMALLINT field_index, _Out_writes_bytes_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                              _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN wstring_to_double( _In_ SQLSMALLINT field_index, _Out_writes_bytes_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                                 _Inout_ SQLLEN* out_buffer_length );
    SQLRETURN wstring_to_long( _In_ SQLSMALLINT field_index, _Out_writes_bytes_(*out_buffer_length) void* buffer, _In_ SQLLEN buffer_length,
                               _Inout_ SQLLEN* out_buffer_length );

    // utility functions for conversions
    unsigned char* get_row( void );
};

//*********************************************************************************************************************************
// Utility
//*********************************************************************************************************************************

// Simple macro to alleviate unused variable warnings.  These are optimized out by the compiler.
// We use this since the unused variables are buried in the PHP_FUNCTION macro.
#define HDB_UNUSED( var )   var;

// do a heap check in debug mode, but only print errors, not all of the allocations
#define MEMCHECK_SILENT 1

// utility functions shared by multiple callers across files
bool convert_string_from_utf16_inplace( _In_ HDB_ENCODING encoding, _Inout_updates_z_(len) char** string, _Inout_ SQLLEN& len);
bool convert_zval_string_from_utf16( _In_ HDB_ENCODING encoding, _Inout_ zval* value_z, _Inout_ SQLLEN& len);
bool validate_string( _In_ char* string, _In_ SQLLEN& len);
bool convert_string_from_utf16( _In_ HDB_ENCODING encoding, _In_reads_bytes_(cchInLen) const SQLWCHAR* inString, _In_ SQLINTEGER cchInLen, _Inout_updates_bytes_(cchOutLen) char** outString, _Out_ SQLLEN& cchOutLen );
SQLWCHAR* utf16_string_from_mbcs_string( _In_ HDB_ENCODING php_encoding, _In_reads_bytes_(mbcs_len) const char* mbcs_string, _In_ unsigned int mbcs_len, _Out_ unsigned int* utf16_len );

//*********************************************************************************************************************************
// Error handling routines and Predefined Errors
//*********************************************************************************************************************************

enum HDB_ERROR_CODES {

    HDB_ERROR_ODBC,
    HDB_ERROR_DRIVER_NOT_INSTALLED,
    HDB_ERROR_CE_DRIVER_REQUIRED,
    HDB_ERROR_CONNECT_INVALID_DRIVER,
    HDB_ERROR_SPECIFIED_DRIVER_NOT_FOUND,
    HDB_ERROR_ZEND_HASH,
    HDB_ERROR_INVALID_PARAMETER_PHPTYPE,
    HDB_ERROR_INVALID_PARAMETER_SQLTYPE,
    HDB_ERROR_INVALID_PARAMETER_ENCODING,
    HDB_ERROR_INPUT_PARAM_ENCODING_TRANSLATE,
    HDB_ERROR_OUTPUT_PARAM_ENCODING_TRANSLATE,
    HDB_ERROR_CONNECT_STRING_ENCODING_TRANSLATE,
    HDB_ERROR_ZEND_STREAM, 
    HDB_ERROR_INPUT_STREAM_ENCODING_TRANSLATE,
    HDB_ERROR_UNKNOWN_SERVER_VERSION,
    HDB_ERROR_FETCH_PAST_END,
    HDB_ERROR_STATEMENT_NOT_EXECUTED,
    HDB_ERROR_NO_FIELDS,
    HDB_ERROR_INVALID_TYPE,
    HDB_ERROR_FETCH_NOT_CALLED,
    HDB_ERROR_NO_DATA,
    HDB_ERROR_FIELD_ENCODING_TRANSLATE,
    HDB_ERROR_ZEND_HASH_CREATE_FAILED,
    HDB_ERROR_NEXT_RESULT_PAST_END,
    HDB_ERROR_UID_PWD_BRACES_NOT_ESCAPED,
    HDB_ERROR_INVALID_OPTION_TYPE_INT,
    HDB_ERROR_INVALID_OPTION_TYPE_STRING,
    HDB_ERROR_CONN_OPTS_WRONG_TYPE,
    HDB_ERROR_INVALID_CONNECTION_KEY,
    HDB_ERROR_MAX_PARAMS_EXCEEDED,
    HDB_ERROR_INVALID_OPTION_KEY,
    HDB_ERROR_INVALID_QUERY_TIMEOUT_VALUE,
    HDB_ERROR_INVALID_OPTION_SCROLLABLE,
    HDB_ERROR_QUERY_STRING_ENCODING_TRANSLATE,
    HDB_ERROR_OUTPUT_PARAM_TRUNCATED,
    HDB_ERROR_INPUT_OUTPUT_PARAM_TYPE_MATCH,
    HDB_ERROR_DATETIME_CONVERSION_FAILED,
    HDB_ERROR_STREAMABLE_TYPES_ONLY,
    HDB_ERROR_STREAM_CREATE,
    HDB_ERROR_MARS_OFF,
    HDB_ERROR_FIELD_INDEX_ERROR,
    HDB_ERROR_BUFFER_LIMIT_EXCEEDED,
    HDB_ERROR_INVALID_BUFFER_LIMIT,
    HDB_ERROR_OUTPUT_PARAM_TYPES_NOT_SUPPORTED,
    HDB_ERROR_INVALID_AKV_AUTHENTICATION_OPTION,
    HDB_ERROR_AKV_AUTH_MISSING,
    HDB_ERROR_AKV_NAME_MISSING,
    HDB_ERROR_AKV_SECRET_MISSING,
    HDB_ERROR_KEYSTORE_INVALID_VALUE,
    HDB_ERROR_DOUBLE_CONVERSION_FAILED,

    // Driver specific error codes starts from here.
    HDB_ERROR_DRIVER_SPECIFIC = 1000,

};

// SQLSTATE for all internal errors
extern SQLCHAR IMSSP[];

// SQLSTATE for all internal warnings
extern SQLCHAR SSPWARN[];

// flags passed to hdb_errors to filter its return values
enum error_handling_flags {
    HDB_ERR_ERRORS,
    HDB_ERR_WARNINGS,
    HDB_ERR_ALL
};

// *** internal error macros and functions ***
// call to retrieve an error from ODBC.  This uses SQLGetDiagRec, so the
// errno is 1 based.  It returns it as an array with 3 members:
// 1/SQLSTATE) sqlstate
// 2/code) driver specific error code
// 3/message) driver specific error message
// The fetch type determines if the indices are numeric, associative, or both.
bool core_hdb_get_odbc_error( _Inout_ hdb_context& ctx, _In_ int record_number, _Inout_ hdb_error_auto_ptr& error, 
                                 _In_ logging_severity severity );

// format and return a driver specfic error
void core_hdb_format_driver_error( _In_ hdb_context& ctx, _In_ hdb_error_const const* custom_error, 
                                      _Out_ hdb_error_auto_ptr& formatted_error, _In_ logging_severity severity, _In_opt_ va_list* args );


// return the message for the HRESULT returned by GetLastError.  Some driver errors use this to
// return the Windows error, e.g, when a UTF-8 <-> UTF-16 conversion fails.
const char* get_last_error_message( _Inout_ DWORD last_error = 0 );

// a wrapper around FormatMessage that can take variadic args rather than a a va_arg pointer
DWORD core_hdb_format_message( _Out_ char* output_buffer, _In_ unsigned output_len, _In_opt_ const char* format, ... );

// convenience functions that overload either a reference or a pointer so we can use
// either in the CHECK_* functions.
inline bool call_error_handler( _Inout_ hdb_context& ctx, _In_ unsigned long hdb_error_code, _In_ bool warning, ... )
{
    va_list print_params;
    va_start( print_params, warning );
    bool ignored = ctx.error_handler()( ctx, hdb_error_code, warning, &print_params );
    va_end( print_params );
    return ignored;
}

inline bool call_error_handler( _Inout_ hdb_context* ctx, _In_ unsigned long hdb_error_code, _In_ bool warning, ... )
{
    va_list print_params;
    va_start( print_params, warning );
    bool ignored = ctx->error_handler()( *ctx, hdb_error_code, warning, &print_params );
    va_end( print_params );
    return ignored;
}

// PHP equivalent of ASSERT.  C asserts cause a dialog to show and halt the process which
// we don't want on a web server

#define HDB_ASSERT( condition, msg, ...)  if( !(condition)) DIE( msg, ## __VA_ARGS__ );
 
#if defined( PHP_DEBUG )                             

#define DEBUG_HDB_ASSERT( condition, msg, ... )    \
    if( !(condition)) {                               \
        DIE (msg, ## __VA_ARGS__ );                      \
    }                 

#else

    #define DEBUG_HDB_ASSERT( condition, msg, ... ) ((void)0)

#endif 

// check to see if the sqlstate is 01004, truncated field retrieved.  Used for retrieving large fields.
inline bool is_truncated_warning( _In_ SQLCHAR* state )
{
#if defined(ZEND_DEBUG)
    if( state == NULL || strnlen_s( reinterpret_cast<char*>( state )) != 5 ) { \
        DIE( "Incorrect SQLSTATE given to is_truncated_warning." ); \
    }
#endif
    return (state[0] == '0' && state[1] == '1' && state[2] == '0' && state [3] == '0' && state [4] == '4');
}

// Macros for handling errors. These macros are simplified if statements that take boilerplate
// code down to a single line to avoid distractions in the code.

#define CHECK_ERROR_EX( unique, condition, context, ssphp, ... )     \
    bool flag##unique = (condition);                                 \
    bool ignored##unique = true;                                       \
    if (flag##unique) {                                              \
        ignored##unique = call_error_handler( context, ssphp, /*warning*/false, ## __VA_ARGS__ ); \
    }  \
    if( !ignored##unique )
    
#define CHECK_ERROR_UNIQUE( unique, condition, context, ssphp, ...) \
    CHECK_ERROR_EX( unique, condition, context, ssphp, ## __VA_ARGS__ )

#define CHECK_ERROR( condition, context, ... )  \
    CHECK_ERROR_UNIQUE( __COUNTER__, condition, context, 0, ## __VA_ARGS__ )

#define CHECK_CUSTOM_ERROR( condition, context, ssphp, ... )  \
    CHECK_ERROR_UNIQUE( __COUNTER__, condition, context, ssphp, ## __VA_ARGS__ )

#define CHECK_SQL_ERROR( result, context, ... )  \
    HDB_ASSERT( result != SQL_INVALID_HANDLE, "Invalid handle returned." ); \
    CHECK_ERROR( result == SQL_ERROR, context, ## __VA_ARGS__ )

#define CHECK_WARNING_AS_ERROR_UNIQUE(  unique, condition, context, ssphp, ... )   \
    bool ignored##unique = true;    \
    if( condition ) { \
        ignored##unique = call_error_handler( context, ssphp, /*warning*/true, ## __VA_ARGS__ ); \
    }   \
    if( !ignored##unique ) 

#define CHECK_SQL_WARNING_AS_ERROR( result, context, ... ) \
    CHECK_WARNING_AS_ERROR_UNIQUE( __COUNTER__,( result == SQL_SUCCESS_WITH_INFO ), context, HDB_ERROR_ODBC, ## __VA_ARGS__ )
    
#define CHECK_SQL_WARNING( result, context, ... )        \
    if( result == SQL_SUCCESS_WITH_INFO ) {              \
        (void)call_error_handler( context, 0, /*warning*/ true, ## __VA_ARGS__ ); \
    }

#define CHECK_CUSTOM_WARNING_AS_ERROR( condition, context, ssphp, ... ) \
    CHECK_WARNING_AS_ERROR_UNIQUE( __COUNTER__, condition, context, ssphp, ## __VA_ARGS__ )
                 
#define CHECK_ZEND_ERROR( zr, ctx, error, ... )  \
    CHECK_ERROR_UNIQUE( __COUNTER__, ( zr == FAILURE ), ctx, error, ## __VA_ARGS__ )  \

#define CHECK_SQL_ERROR_OR_WARNING( result, context, ... ) \
    HDB_ASSERT( result != SQL_INVALID_HANDLE, "Invalid handle returned." );  \
    bool ignored = true;                                   \
    if( result == SQL_ERROR ) {                            \
        ignored = call_error_handler( context, HDB_ERROR_ODBC, false, ##__VA_ARGS__ ); \
    }                                                      \
    else if( result == SQL_SUCCESS_WITH_INFO ) {           \
        ignored = call_error_handler( context, HDB_ERROR_ODBC, true, ##__VA_ARGS__ ); \
    }                                                      \
    if( !ignored )
  
// throw an exception after it has been hooked into the custom error handler
#define THROW_CORE_ERROR( ctx, custom, ... ) \
  (void)call_error_handler( ctx, custom, /*warning*/ false, ## __VA_ARGS__ ); \
  throw core::CoreException();

//*********************************************************************************************************************************
// ODBC/Zend function wrappers
//*********************************************************************************************************************************

namespace core {

    // base exception for the driver
    struct CoreException : public std::exception {

      CoreException()
      {
      }
    };

    inline void check_for_mars_error( _Inout_ hdb_stmt* stmt, _In_ SQLRETURN r )
    {
        // We check for the 'connection busy' error caused by having MultipleActiveResultSets off
        // and return a more helpful message prepended to the ODBC errors if that error occurs
        if( !SQL_SUCCEEDED( r )) {

            SQLCHAR err_msg[ SQL_MAX_ERROR_MESSAGE_LENGTH + 1 ] = { '\0' };
            SQLSMALLINT len = 0;
            
            SQLRETURN rtemp = ::SQLGetDiagField( stmt->handle_type(), stmt->handle(), 1, SQL_DIAG_MESSAGE_TEXT, 
                                             err_msg, SQL_MAX_ERROR_MESSAGE_LENGTH, &len );

            CHECK_SQL_ERROR_OR_WARNING( rtemp, stmt ) {
         
                throw CoreException();
            }

            // the message returned by ODBC Driver for SQL Server
            const std::string connection_busy_error( ODBC_CONNECTION_BUSY_ERROR );
            const std::string returned_error( reinterpret_cast<char*>( err_msg ));

            if(( returned_error.find( connection_busy_error ) != std::string::npos )) {
             
                THROW_CORE_ERROR( stmt, HDB_ERROR_MARS_OFF );
            }
        }
    }

    // *** ODBC wrappers ***

    // wrap the ODBC functions to throw exceptions rather than use the return value to signal errors
    // some of the signatures have been altered to be more convenient since the return value is no longer
    // required to return the status of the call (e.g., SQLNumResultCols).
    // These functions take the hdb_context type.  However, since the error handling code can alter
    // the context to hold the error, they are not passed as const.

    inline SQLRETURN SQLGetDiagField( _Inout_ hdb_context* ctx, _In_ SQLSMALLINT record_number, _In_ SQLSMALLINT diag_identifier, 
                                      _Out_writes_opt_(buffer_length) SQLPOINTER diag_info_buffer, _In_ SQLSMALLINT buffer_length,
                                      _Out_opt_ SQLSMALLINT* out_buffer_length )
    {
        SQLRETURN r = ::SQLGetDiagField( ctx->handle_type(), ctx->handle(), record_number, diag_identifier, 
                                       diag_info_buffer, buffer_length, out_buffer_length );

        CHECK_SQL_ERROR_OR_WARNING( r, ctx ) {
            throw CoreException();
        }

        return r;
    }

    inline void SQLAllocHandle( _In_ SQLSMALLINT HandleType, _Inout_ hdb_context& InputHandle, 
                                _Out_ SQLHANDLE* OutputHandlePtr )
    {
        SQLRETURN r;
        r = ::SQLAllocHandle( HandleType, InputHandle.handle(), OutputHandlePtr );
        CHECK_SQL_ERROR_OR_WARNING( r, InputHandle ) {
            throw CoreException();
        }
    }

    inline void SQLBindParameter( _Inout_ hdb_stmt*          stmt, 
                                  _In_ SQLUSMALLINT             ParameterNumber,
                                  _In_ SQLSMALLINT              InputOutputType,
                                  _In_ SQLSMALLINT              ValueType,
                                  _In_ SQLSMALLINT              ParameterType,
                                  _In_ SQLULEN                  ColumnSize,
                                  _In_ SQLSMALLINT              DecimalDigits,
                                  _Inout_opt_ SQLPOINTER        ParameterValuePtr,
                                  _Inout_ SQLLEN                BufferLength,
                                  _Inout_ SQLLEN *              StrLen_Or_IndPtr
                                  )
    {
        SQLRETURN r;
        r = ::SQLBindParameter( stmt->handle(), ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, 
                                DecimalDigits, ParameterValuePtr, BufferLength, StrLen_Or_IndPtr );
        
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

    inline void SQLCloseCursor( _Inout_ hdb_stmt* stmt )
    {
        SQLRETURN r = ::SQLCloseCursor( stmt->handle() );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

    inline void SQLColAttribute( _Inout_ hdb_stmt* stmt, _In_ SQLUSMALLINT field_index, _In_ SQLUSMALLINT field_identifier, 
                                 _Out_writes_bytes_opt_(buffer_length) SQLPOINTER field_type_char, _In_ SQLSMALLINT buffer_length,
                                 _Out_opt_ SQLSMALLINT* out_buffer_length, _Out_opt_ SQLLEN* field_type_num )
    {
        SQLRETURN r = ::SQLColAttribute( stmt->handle(), field_index, field_identifier, field_type_char,
                                         buffer_length, out_buffer_length, field_type_num );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

    inline void SQLColAttributeW( _Inout_ hdb_stmt* stmt, _In_ SQLUSMALLINT field_index, _In_ SQLUSMALLINT field_identifier,
                                  _Out_writes_bytes_opt_(buffer_length) SQLPOINTER field_type_char, _In_ SQLSMALLINT buffer_length,
                                  _Out_opt_ SQLSMALLINT* out_buffer_length, _Out_opt_ SQLLEN* field_type_num )
    {
        SQLRETURN r = ::SQLColAttributeW( stmt->handle(), field_index, field_identifier, field_type_char,
                                          buffer_length, out_buffer_length, field_type_num );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

    inline void SQLDescribeCol( _Inout_ hdb_stmt* stmt, _In_ SQLSMALLINT colno, _Out_writes_opt_(col_name_length) SQLCHAR* col_name, _In_ SQLSMALLINT col_name_length,
                                _Out_opt_ SQLSMALLINT* col_name_length_out, _Out_opt_ SQLSMALLINT* data_type, _Out_opt_ SQLULEN* col_size,
                                _Out_opt_ SQLSMALLINT* decimal_digits, _Out_opt_ SQLSMALLINT* nullable )
    {
        SQLRETURN r;
        r = ::SQLDescribeCol( stmt->handle(), colno, col_name, col_name_length, col_name_length_out, 
                              data_type, col_size, decimal_digits, nullable);
         
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

	inline void SQLDescribeColW( _Inout_ hdb_stmt* stmt, _In_ SQLSMALLINT colno, _Out_writes_opt_(col_name_length) SQLWCHAR* col_name, _In_ SQLSMALLINT col_name_length,
                                 _Out_opt_ SQLSMALLINT* col_name_length_out, _Out_opt_ SQLSMALLINT* data_type, _Out_opt_ SQLULEN* col_size,
                                 _Out_opt_ SQLSMALLINT* decimal_digits, _Out_opt_ SQLSMALLINT* nullable )
	{
		SQLRETURN r;
		r = ::SQLDescribeColW( stmt->handle(), colno, col_name, col_name_length, col_name_length_out,
                               data_type, col_size, decimal_digits, nullable );

		CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
			throw CoreException();
		}
	}

    inline void SQLDescribeParam( _Inout_ hdb_stmt* stmt, _In_ SQLSMALLINT paramno, _Out_opt_ SQLSMALLINT* data_type, _Out_opt_ SQLULEN* col_size,
        _Out_opt_ SQLSMALLINT* decimal_digits, _Out_opt_ SQLSMALLINT* nullable )
    {
        SQLRETURN r;
        r = ::SQLDescribeParam( stmt->handle(), paramno, data_type, col_size, decimal_digits, nullable );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

    inline void SQLNumParams( _Inout_ hdb_stmt* stmt, _Out_opt_ SQLSMALLINT* num_params)
    {
        SQLRETURN r;
        r = ::SQLNumParams( stmt->handle(), num_params );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

    inline void SQLEndTran( _In_ SQLSMALLINT handleType, _Inout_ hdb_conn* conn, _In_ SQLSMALLINT completionType )
    {
        SQLRETURN r = ::SQLEndTran( handleType, conn->handle(), completionType );
        
        CHECK_SQL_ERROR_OR_WARNING( r, conn ) {
            throw CoreException();
        }
    }

    // SQLExecDirect returns the status code since it returns either SQL_NEED_DATA or SQL_NO_DATA besides just errors/success    
    inline SQLRETURN SQLExecDirect( _Inout_ hdb_stmt* stmt, _In_ char* sql )
    {
        SQLRETURN r = ::SQLExecDirect( stmt->handle(), reinterpret_cast<SQLCHAR*>( sql ), SQL_NTS );
        
        check_for_mars_error( stmt, r );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {

            throw CoreException();
        }
        return r;
    }

    inline SQLRETURN SQLExecDirectW( _Inout_ hdb_stmt* stmt, _In_ SQLWCHAR* wsql )
    {
        SQLRETURN r;
        r = ::SQLExecDirectW( stmt->handle(), reinterpret_cast<SQLWCHAR*>( wsql ), SQL_NTS );

        check_for_mars_error( stmt, r );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
        return r;
    }

    // SQLExecute returns the status code since it returns either SQL_NEED_DATA or SQL_NO_DATA besides just errors/success
    inline SQLRETURN SQLExecute( _Inout_ hdb_stmt* stmt )
    {
        SQLRETURN r;
        r = ::SQLExecute( stmt->handle() );
   
        check_for_mars_error( stmt, r );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }

        return r;
    }

    inline SQLRETURN SQLFetchScroll( _Inout_ hdb_stmt* stmt, _In_ SQLSMALLINT fetch_orientation, _In_ SQLLEN fetch_offset )
    {
        SQLRETURN r = ::SQLFetchScroll( stmt->handle(), fetch_orientation, fetch_offset );
        
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
        return r;
    }


    // wrap SQLFreeHandle and report any errors, but don't actually signal an error to the calling routine
    inline void SQLFreeHandle( _Inout_ hdb_context& ctx )
    {
        SQLRETURN r;
        r = ::SQLFreeHandle( ctx.handle_type(), ctx.handle() );
        CHECK_SQL_ERROR_OR_WARNING( r, ctx ) {}
    }

    inline void SQLGetStmtAttr( _Inout_ hdb_stmt* stmt, _In_ SQLINTEGER attr, _Out_writes_opt_(buf_len) void* value_ptr, _In_ SQLINTEGER buf_len, _Out_opt_ SQLINTEGER* str_len)
    {
        SQLRETURN r;
        r = ::SQLGetStmtAttr( stmt->handle(), attr, value_ptr, buf_len, str_len );
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }

    inline SQLRETURN SQLGetData( _Inout_ hdb_stmt* stmt, _In_ SQLUSMALLINT field_index, _In_ SQLSMALLINT target_type,
                                 _Out_writes_opt_(buffer_length) void* buffer, _In_ SQLLEN buffer_length, _Out_opt_ SQLLEN* out_buffer_length,
                                 _In_ bool handle_warning )
    {
        SQLRETURN r = ::SQLGetData( stmt->handle(), field_index, target_type, buffer, buffer_length, out_buffer_length );

        if( r == SQL_NO_DATA )
            return r;
        
        CHECK_SQL_ERROR( r, stmt ) {
            throw CoreException();
        }

        if( handle_warning ) {
            CHECK_SQL_WARNING_AS_ERROR( r, stmt ) {
                throw CoreException();
            }
        }

        return r;
    }

   
    inline void SQLGetInfo( _Inout_ hdb_conn* conn, _In_ SQLUSMALLINT info_type, _Out_writes_bytes_opt_(buffer_len) SQLPOINTER info_value, _In_ SQLSMALLINT buffer_len,
                     _Out_opt_ SQLSMALLINT* str_len )
    {
        SQLRETURN r;
        r = ::SQLGetInfo( conn->handle(), info_type, info_value, buffer_len, str_len );
        
        CHECK_SQL_ERROR_OR_WARNING( r, conn ) {
            throw CoreException();
        }
    }


    inline void SQLGetTypeInfo( _Inout_ hdb_stmt* stmt, _In_ SQLUSMALLINT data_type )
    {
        SQLRETURN r;
        r = ::SQLGetTypeInfo( stmt->handle(), data_type );
        
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }


    // SQLMoreResults returns the status code since it returns SQL_NO_DATA when there is no more data in a result set.
    inline SQLRETURN SQLMoreResults( _Inout_ hdb_stmt* stmt )
    {
        SQLRETURN r = ::SQLMoreResults( stmt->handle() );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }

        return r;
    }

    inline SQLSMALLINT SQLNumResultCols( _Inout_ hdb_stmt* stmt )
    {
        SQLRETURN r;
        SQLSMALLINT num_cols;
        r = ::SQLNumResultCols( stmt->handle(), &num_cols );

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
        
        return num_cols;
    }

    // SQLParamData returns the status code since it returns either SQL_NEED_DATA or SQL_NO_DATA when there are more
    // parameters or when the parameters are all processed.
    inline SQLRETURN SQLParamData( _Inout_ hdb_stmt* stmt, _Out_opt_ SQLPOINTER* value_ptr_ptr )
    {
        SQLRETURN r;
        r = ::SQLParamData( stmt->handle(), value_ptr_ptr );
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
        return r;
    }

    inline void SQLPrepareW( _Inout_ hdb_stmt* stmt, _In_reads_(sql_len) SQLWCHAR * sql, _In_ SQLINTEGER sql_len )
    {
        SQLRETURN r;
        r = ::SQLPrepareW( stmt->handle(), sql, sql_len );
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }


    inline void SQLPutData( _Inout_ hdb_stmt* stmt, _In_reads_(strlen_or_ind) SQLPOINTER data_ptr, _In_ SQLLEN strlen_or_ind )
    {
        SQLRETURN r;
        r = ::SQLPutData( stmt->handle(), data_ptr, strlen_or_ind );
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }


    inline SQLLEN SQLRowCount( _Inout_ hdb_stmt* stmt )
    {
        SQLRETURN r;
        SQLLEN rows_affected;

        r = ::SQLRowCount( stmt->handle(), &rows_affected );
        
        // On Linux platform
        // DriverName: libmsodbcsql-13.0.so.0.0
        // DriverODBCVer: 03.52
        // DriverVer: 13.00.0000
        // unixODBC: 2.3.1
        // r = ::SQLRowCount( stmt->handle(), &rows_affected );  
        // returns r=-1 for an empty result set. 
#ifndef _WIN32
        if( r == -1 && rows_affected == -1 )
           return 0;
#endif // !_WIN32

        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }

        return rows_affected;
    }


    inline void SQLSetConnectAttr( _Inout_ hdb_context& ctx, _In_ SQLINTEGER attr, _In_reads_bytes_opt_(str_len) SQLPOINTER value_ptr, _In_ SQLINTEGER str_len )
    {
        SQLRETURN r;
        r = ::SQLSetConnectAttr( ctx.handle(), attr, value_ptr, str_len );
        
        CHECK_SQL_ERROR_OR_WARNING( r, ctx ) {
            throw CoreException();
        }
    }

    inline void SQLSetDescField( _Inout_ hdb_stmt* stmt, _In_ SQLSMALLINT rec_num, _In_ SQLSMALLINT fld_id, _In_reads_bytes_opt_( str_len ) SQLPOINTER value_ptr, _In_ SQLINTEGER str_len  )
    {
        SQLRETURN r;
        SQLHDESC hIpd = NULL;
        core::SQLGetStmtAttr( stmt, SQL_ATTR_IMP_PARAM_DESC, &hIpd, 0, 0 );
        if( value_ptr ) {
            r = ::SQLSetDescField( hIpd, rec_num, fld_id, value_ptr, str_len );
            CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
                throw CoreException();
            }
        }
    }

    inline void SQLSetEnvAttr( _Inout_ hdb_context& ctx, _In_ SQLINTEGER attr, _In_reads_bytes_opt_(str_len) SQLPOINTER value_ptr, _In_ SQLINTEGER str_len )
    {
        SQLRETURN r;
        r = ::SQLSetEnvAttr( ctx.handle(), attr, value_ptr, str_len );
        CHECK_SQL_ERROR_OR_WARNING( r, ctx ) {
            throw CoreException();
        }
    }

    inline void SQLSetConnectAttr( _Inout_ hdb_conn* conn, _In_ SQLINTEGER attribute, _In_reads_bytes_opt_(value_len) SQLPOINTER value_ptr, _In_ SQLINTEGER value_len )
    {
        SQLRETURN r = ::SQLSetConnectAttr( conn->handle(), attribute, value_ptr, value_len ); 
        
        CHECK_SQL_ERROR_OR_WARNING( r, conn ) {
            throw CoreException();
        }
    }
        
    inline void SQLSetStmtAttr( _Inout_ hdb_stmt* stmt, _In_ SQLINTEGER attr, _In_reads_(str_len) SQLPOINTER value_ptr, _In_ SQLINTEGER str_len )
    {
        SQLRETURN r;
        r = ::SQLSetStmtAttr( stmt->handle(), attr, value_ptr, str_len );
        CHECK_SQL_ERROR_OR_WARNING( r, stmt ) {
            throw CoreException();
        }
    }


    // *** zend wrappers ***

	//zend_resource_dtor sets the type of destroyed resources to -1
	#define RSRC_INVALID_TYPE -1

	// wrapper for ZVAL_STRINGL macro. ZVAL_STRINGL always allocates memory when initialzing new string from char string
	// so allocated memory inside of value_z should be released before assigning it to the new string
	inline void hdb_zval_stringl( _Inout_ zval* value_z, _In_reads_(str_len) const char* str, _In_ const std::size_t str_len)
	{
		if (Z_TYPE_P(value_z) == IS_STRING && Z_STR_P(value_z) != NULL) {
			zend_string* temp_zstr = zend_string_init(str, str_len, 0);
			zend_string_release(Z_STR_P(value_z));
			ZVAL_NEW_STR(value_z, temp_zstr);
		}
		else {
			ZVAL_STRINGL(value_z, str, str_len);
		}
	}


    // exception thrown when a zend function wrapped here fails.

    // wrappers for the zend functions called by our driver.  These functions hook into the error reporting of our driver and throw
    // exceptions when an error occurs.  They are prefaced with hdb_<zend_function_name> because many of the zend functions are
    // actually macros that call other functions, so the hdb_ is necessary to differentiate them from the macro system.
    // If there is a zend function in the source that isn't found here, it is because it returns void and there is no error
    // that can be thrown from it.

    inline void hdb_add_index_zval( _Inout_ hdb_context& ctx, _Inout_ zval* array, _In_ zend_ulong index, _In_ zval* value) 
    {
        int zr = ::add_index_zval( array, index, value );
        CHECK_ZEND_ERROR( zr, ctx, HDB_ERROR_ZEND_HASH ) {
            throw CoreException();
        }
    }

    inline void hdb_add_next_index_zval( _Inout_ hdb_context& ctx, _Inout_ zval* array, _In_ zval* value) 
    {
        int zr = ::add_next_index_zval( array, value );
        CHECK_ZEND_ERROR( zr, ctx, HDB_ERROR_ZEND_HASH ) {
            throw CoreException();
        }
    }

    inline void hdb_add_assoc_null( _Inout_ hdb_context& ctx, _Inout_ zval* array_z, _In_ const char* key )
    {
        // int zr = ::add_assoc_null( array_z, key );
        // CHECK_ZEND_ERROR (zr, ctx, HDB_ERROR_ZEND_HASH ) {
        //     throw CoreException();
        // }
    }

    inline void hdb_add_assoc_long( _Inout_ hdb_context& ctx, _Inout_ zval* array_z, _In_ const char* key, _In_ zend_long val )
    {
        // int zr = ::add_assoc_long( array_z, key, val );
        // CHECK_ZEND_ERROR (zr, ctx, HDB_ERROR_ZEND_HASH ) {
        //     throw CoreException();
        // }
    }

    inline void hdb_add_assoc_string( _Inout_ hdb_context& ctx, _Inout_ zval* array_z, _In_ const char* key, _Inout_z_ char* val, _In_ bool duplicate )
    {
        // int zr = ::add_assoc_string(array_z, key, val);
        // CHECK_ZEND_ERROR (zr, ctx, HDB_ERROR_ZEND_HASH ) {
        //     throw CoreException();
        // }
        if (duplicate == 0) {
            hdb_free(val);
        }
    }

    inline void hdb_array_init( _Inout_ hdb_context& ctx, _Out_ zval* new_array) 
    {
        array_init(new_array);
        int zr = SUCCESS;
        CHECK_ZEND_ERROR( zr, ctx, HDB_ERROR_ZEND_HASH ) {
            throw CoreException();
        }
    }

    inline void hdb_php_stream_from_zval_no_verify( _Inout_ hdb_context& ctx, _Outref_result_maybenull_ php_stream*& stream, _In_opt_ zval* stream_z )
    {
        // this duplicates the macro php_stream_from_zval_no_verify, which we can't use because it has an assignment
        php_stream_from_zval_no_verify( stream, stream_z );
        CHECK_CUSTOM_ERROR( stream == NULL, ctx, HDB_ERROR_ZEND_STREAM ) {
            throw CoreException();
        }
    }

	inline void hdb_zend_hash_get_current_data( _In_ hdb_context& ctx, _In_ HashTable* ht, _Outref_result_maybenull_ zval*& output_data)
	{
		int zr = (output_data = ::zend_hash_get_current_data(ht)) != NULL ? SUCCESS : FAILURE;
		CHECK_ZEND_ERROR( zr, ctx, HDB_ERROR_ZEND_HASH ) {
			throw CoreException();
		}
	}

    inline void hdb_zend_hash_get_current_data_ptr( _Inout_ hdb_context& ctx, _In_ HashTable* ht, _Outref_result_maybenull_ void*& output_data)
    {
        int zr = (output_data = ::zend_hash_get_current_data_ptr(ht)) != NULL ? SUCCESS : FAILURE;
        CHECK_ZEND_ERROR(zr, ctx, HDB_ERROR_ZEND_HASH) {
            throw CoreException();
        }
    }

    inline void hdb_zend_hash_index_del( _Inout_ hdb_context& ctx, _Inout_ HashTable* ht, _In_ zend_ulong index )
    {
        int zr = ::zend_hash_index_del( ht, index );
        CHECK_ZEND_ERROR( zr, ctx, HDB_ERROR_ZEND_HASH ) {
            throw CoreException();
        }
    }
   
    inline void hdb_zend_hash_index_update( _Inout_ hdb_context& ctx, _Inout_ HashTable* ht, _In_ zend_ulong index, _In_ zval* data_z )
    {
        int zr = (data_z = ::zend_hash_index_update(ht, index, data_z)) != NULL ? SUCCESS : FAILURE;
        CHECK_ZEND_ERROR( zr, ctx, HDB_ERROR_ZEND_HASH ) {
            throw CoreException();
        }
    }

    inline void hdb_zend_hash_index_update_ptr( _Inout_ hdb_context& ctx, _Inout_ HashTable* ht, _In_ zend_ulong index, _In_ void* pData)
    {
        int zr = (pData = ::zend_hash_index_update_ptr(ht, index, pData)) != NULL ? SUCCESS : FAILURE;
        CHECK_ZEND_ERROR(zr, ctx, HDB_ERROR_ZEND_HASH) {
            throw CoreException();
        }
    }


	inline void hdb_zend_hash_index_update_mem( _Inout_ hdb_context& ctx, _Inout_ HashTable* ht, _In_ zend_ulong index, _In_reads_bytes_(size) void* pData, _In_ std::size_t size)
	{
		int zr = (pData = ::zend_hash_index_update_mem(ht, index, pData, size)) != NULL ? SUCCESS : FAILURE;
		CHECK_ZEND_ERROR(zr, ctx, HDB_ERROR_ZEND_HASH) {
			throw CoreException();
		}
	}
    
    inline void hdb_zend_hash_next_index_insert( _Inout_ hdb_context& ctx, _Inout_ HashTable* ht, _In_ zval* data )
    {
        int zr = (data = ::zend_hash_next_index_insert(ht, data)) != NULL ? SUCCESS : FAILURE;
        CHECK_ZEND_ERROR( zr, ctx, HDB_ERROR_ZEND_HASH ) {
            throw CoreException();
        }
    }

	inline void hdb_zend_hash_next_index_insert_mem( _Inout_ hdb_context& ctx, _In_ HashTable* ht, _In_reads_bytes_(data_size) void* data, _In_ uint data_size)
	{
		int zr = (data = ::zend_hash_next_index_insert_mem(ht, data, data_size)) != NULL ? SUCCESS : FAILURE;
		CHECK_ZEND_ERROR(zr, ctx, HDB_ERROR_ZEND_HASH) {
			throw CoreException();
		}
	}

    inline void hdb_zend_hash_next_index_insert_ptr( _Inout_ hdb_context& ctx, _Inout_ HashTable* ht, _In_ void* data)
    {
        int zr = (data = ::zend_hash_next_index_insert_ptr(ht, data)) != NULL ? SUCCESS : FAILURE;
        CHECK_ZEND_ERROR(zr, ctx, HDB_ERROR_ZEND_HASH) {
            throw CoreException();
        }
    }
   
    inline void hdb_zend_hash_init(hdb_context& ctx, _Inout_ HashTable* ht, _Inout_ uint32_t initial_size,
        _In_ dtor_func_t dtor_fn, _In_ zend_bool persistent )
    {
        ::zend_hash_init(ht, initial_size, NULL, dtor_fn, persistent);
    }

template <typename Statement>
hdb_stmt* allocate_stmt( _In_ hdb_conn* conn, _In_ SQLHANDLE h, _In_ error_callback e, _In_ void* driver )
{
    return new ( hdb_malloc( sizeof( Statement ))) Statement( conn, h, e, driver );
}

template <typename Connection>
hdb_conn* allocate_conn( _In_ SQLHANDLE h, _In_ error_callback e, _In_ void* driver )
{
    return new ( hdb_malloc( sizeof( Connection ))) Connection( h, e, driver );
}

} // namespace core

template <unsigned int Attr>
struct str_conn_attr_func {

    static void func( connection_option const* /*option*/, zval* value, _Inout_ hdb_conn* conn, std::string& /*conn_str*/ )
    {
        try {
            core::SQLSetConnectAttr( conn, Attr, reinterpret_cast<SQLPOINTER>( Z_STRVAL_P( value )), static_cast<SQLINTEGER>( Z_STRLEN_P( value )) );
        }
        catch ( core::CoreException& ) {
            throw;
        }
    }
};

#endif  // CORE_HDB_H
