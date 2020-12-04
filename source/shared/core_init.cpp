//---------------------------------------------------------------------------------------------------------------------------------
// File: core_init.cpp
//
// Contents: common initialization routines shared by PDO and hdb
//
//---------------------------------------------------------------------------------------------------------------------------------

#include "core_hdb.h"

// module global variables (initialized in minit and freed in mshutdown)
HMODULE g_hdb_hmodule = NULL;
bool isVistaOrGreater;


// core_hdb_minit
// Module initialization
// This function is called once per execution by the driver layer's MINIT function.
// The primary responsibility of this function is to allocate the two environment 
// handles used by core_hdb_connect to allocate either a pooled or non pooled ODBC 
// connection handle.
// Parameters:
// henv_cp  - Environment handle for pooled connection.
// henv_ncp - Environment handle for non-pooled connection.
// err      - Driver specific error handler which handles any errors during initialization.
void core_hdb_minit( _Outptr_ hdb_context** henv_cp, _Inout_ hdb_context** henv_ncp, _In_ error_callback err, _In_z_ const char* driver_func TSRMLS_DC )
{
    HDB_STATIC_ASSERT( sizeof( hdb_sqltype ) == sizeof( zend_long ) );
    HDB_STATIC_ASSERT( sizeof( hdb_phptype ) == sizeof( zend_long ));

#ifndef _WIN32
    // set locale from environment
    // this is necessary for ODBC and MUST be done before connection
    setlocale(LC_ALL, "");
#endif

    *henv_cp = *henv_ncp = SQL_NULL_HANDLE; // initialize return values to NULL

    try {

    SQLHANDLE henv = SQL_NULL_HANDLE;
    SQLRETURN r;

    // allocate the non pooled environment handle
    // we can't use the wrapper in core_hdb.h since we don't have a context on which to base errors, so
    // we use the direct ODBC function.
    r = ::SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv );
    if( !SQL_SUCCEEDED( r )) {
        throw core::CoreException();
    }

    *henv_ncp = new hdb_context( henv, SQL_HANDLE_ENV, err, NULL );
    (*henv_ncp)->set_func( driver_func );
    
    // set to ODBC 3
    core::SQLSetEnvAttr( **henv_ncp, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>( SQL_OV_ODBC3 ), SQL_IS_INTEGER 
                         TSRMLS_CC );

    // disable connection pooling
    core::SQLSetEnvAttr( **henv_ncp, SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>( SQL_CP_OFF ), 
                         SQL_IS_UINTEGER TSRMLS_CC );

    // allocate the pooled envrionment handle
    // we can't use the wrapper in core_hdb.h since we don't have a context on which to base errors, so
    // we use the direct ODBC function.
    r = ::SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv );
    if( !SQL_SUCCEEDED( r )) {
        throw core::CoreException();
    }

    *henv_cp = new hdb_context( henv, SQL_HANDLE_ENV, err, NULL );
    (*henv_cp)->set_func( driver_func );

    // set to ODBC 3
    core::SQLSetEnvAttr( **henv_cp, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>( SQL_OV_ODBC3 ), SQL_IS_INTEGER TSRMLS_CC);

    // enable connection pooling
    core:: SQLSetEnvAttr( **henv_cp, SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>( SQL_CP_ONE_PER_HENV ), 
                              SQL_IS_UINTEGER TSRMLS_CC );

    }
    catch( core::CoreException& e ) {

        LOG( SEV_ERROR, "core_hdb_minit: Failed to allocate environment handles." );

        if( *henv_ncp != NULL ) {
            // free the ODBC env handle allocated just above
            SQLFreeHandle( SQL_HANDLE_ENV, **henv_ncp );
            delete *henv_ncp;   // free the memory for the hdb_context (it comes from the C heap, not PHP's heap)
            *henv_ncp = NULL;
        }
        if( *henv_cp != NULL ) {
            // free the ODBC env handle allocated just above
            SQLFreeHandle( SQL_HANDLE_ENV, **henv_cp );
            delete *henv_cp;   // free the memory for the hdb_context (it comes from the C heap, not PHP's heap)
            *henv_cp = NULL;
        }

        throw e;        // rethrow for the driver to catch
    }
    catch( std::bad_alloc& e ) {

        LOG( SEV_ERROR, "core_hdb_minit: Failed memory allocation for environment handles." );

        if( *henv_ncp != NULL ) {
            SQLFreeHandle( SQL_HANDLE_ENV, **henv_ncp );
            delete *henv_ncp;
            *henv_ncp = NULL;
        }
        if( *henv_cp ) {
            SQLFreeHandle( SQL_HANDLE_ENV, **henv_cp );
            delete *henv_cp;
            *henv_cp = NULL;
        }

        throw e;        // rethrow for the driver to catch
    }
}

// core_hdb_mshutdown
// Module shutdown function
// Free the environment handles allocated in MINIT and unregister our stream wrapper.
// Resource types and constants are automatically released since we don't flag them as
// persistent when they are registered.
// Parameters:
// henv_cp -  Pooled environment handle.
// henv_ncp - Non-pooled environment handle.
void core_hdb_mshutdown( _Inout_ hdb_context& henv_cp, _Inout_ hdb_context& henv_ncp )
{
    if( henv_ncp != SQL_NULL_HANDLE ) {

        henv_ncp.invalidate();
    }
	delete &henv_ncp;

    if( henv_cp != SQL_NULL_HANDLE ) {

        henv_cp.invalidate();
    }
	delete &henv_cp;

    return;
}
