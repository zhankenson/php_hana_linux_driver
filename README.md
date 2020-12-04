# Drivers for SAP HANA
The drivers for PHP for SAP HANA are PHP extensions that allow for the 
reading and writing of SAP HANA data from within PHP scripts. These drivers 
rely on the HANA ODBC driver to handle the low-level communication.

## Prerequisites
Install PHP on your Linux env:
* install php and php-devel, make sure the php version between 7.* and 8.0
* gcc version must be grater than 4.9 (for php version > 7.2)

Configure odbc library:
* install HANA client to /usr/sap/hdbclient, configure path /usr/sap/hdbclient to environment parameter LD_LIBRARY_PATH and LIBRARY_PATH to make sure odbc shared library libodbcHDB.so can be used during run and compile.

## Building and Installing the php extension
* fetch project, enter the project directory
* run build.sh to build PHP extension for SAP HANA
* After running this script without errors, you will find the php extension in "{your git repo directory}/build/modules/"
* configure the installed extension to php.ini

## Sample Test 
Under /test directory connect_and_query.php contains basic functions sample including:
 * hdb_connect 
 * hdb_query
 * hdb_get_field
 * hdb_num_fields
 * hdb_prepare
 * hdb_execute
 * hdb_fetch_array
 * hdb_errors
 * hdb_free_stmt
 * hdb_close

## Dockerfile (example for docker php offical mirror)
```
RUN mkdir -p /var/php_hana_linux_driver
COPY ./HDB/php80_hana_linux_driver /var/php_hana_linux_driver
RUN cd /var/php_hana_linux_driver \ 
    && /bin/sh build.sh \
    && docker-php-ext-enable hdb
```

## Fix
* 1.in /usr/local/include/php/Zend/zend_list.h, zend_list_close is return void
* 2.in /usr/local/include/php/Zend/zend_API.h, add_assoc_* is return void
* 3.remove TSRMLS_CC.this macro was defined empty in PHP 7.x, and removed in PHP 8.x.