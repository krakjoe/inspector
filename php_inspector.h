/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2018 Joe Watkins                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_INSPECTOR_H
#define PHP_INSPECTOR_H

extern zend_module_entry inspector_module_entry;
#define phpext_inspector_ptr &inspector_module_entry

#define PHP_INSPECTOR_VERSION "0.2.0dev"
#define PHP_INSPECTOR_EXTNAME "inspector"

#ifdef PHP_WIN32
#	define PHP_INSPECTOR_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_INSPECTOR_API __attribute__ ((visibility("default")))
#else
#	define PHP_INSPECTOR_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

typedef enum {
	PHP_INSPECTOR_ROOT_PENDING,
	PHP_INSPECTOR_ROOT_REGISTERED,
	PHP_INSPECTOR_ROOT_LOCALS,
} php_inspector_root_t;

typedef enum {
	PHP_INSPECTOR_TABLE_FILE,
	PHP_INSPECTOR_TABLE_FUNCTION,
	PHP_INSPECTOR_TABLE_CLASS,
} php_inspector_table_t;

#if defined(ZTS) && defined(COMPILE_DL_INSPECTOR)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

HashTable* php_inspector_table(
	php_inspector_root_t root, php_inspector_table_t type, 
	zend_string *key, zend_bool create);

void php_inspector_table_drop(
	php_inspector_root_t root, php_inspector_table_t type, 
	zend_string *key);

void php_inspector_table_insert(
	php_inspector_root_t root, php_inspector_table_t type, 
	zend_string *key, zval *zv);

void php_inspector_function_map(
	zend_function *source, zend_function *destination);

void php_inspector_file_map(
	zend_function *source, zend_function *destination);

zend_function* php_inspector_function_find(zend_function *function);

zend_function* php_inspector_file_find(zend_function *function);

#endif	/* PHP_INSPECTOR_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
