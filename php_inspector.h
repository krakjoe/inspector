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

#define PHP_INSPECTOR_VERSION "0.1.0dev"
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
} php_inspector_root_t;

typedef enum {
	PHP_INSPECTOR_TABLE_FILE,
	PHP_INSPECTOR_TABLE_FUNCTION,
	PHP_INSPECTOR_TABLE_CLASS,
} php_inspector_table_t;

typedef struct _php_inspector_tables_t {
	HashTable file;
	HashTable function;
	HashTable class;
} php_inspector_tables_t;

ZEND_BEGIN_MODULE_GLOBALS(inspector)
	php_inspector_tables_t pending;
	php_inspector_tables_t registered;
ZEND_END_MODULE_GLOBALS(inspector)

ZEND_DECLARE_MODULE_GLOBALS(inspector);

#ifdef ZTS
#define PIG(v) TSRMG(inspector_globals_id, zend_inspector_globals *, v)
#else
#define PIG(v) (inspector_globals.v)
#endif

static zend_always_inline HashTable* php_inspector_table_select(php_inspector_root_t root, php_inspector_table_t type) {
	php_inspector_tables_t *tables;

	switch (root) {
		case PHP_INSPECTOR_ROOT_PENDING:
			tables = &PIG(pending);
		break;

		case PHP_INSPECTOR_ROOT_REGISTERED:
			tables = &PIG(registered);
		break;

		default:
			return NULL;
	}

	switch (type) {
		case PHP_INSPECTOR_TABLE_FILE:
			return &tables->file;
		case PHP_INSPECTOR_TABLE_FUNCTION:
			return &tables->function;
		case PHP_INSPECTOR_TABLE_CLASS:
			return &tables->class;
	}

	return NULL;
}

static zend_always_inline HashTable* php_inspector_table_create(php_inspector_root_t root, php_inspector_table_t type, 	zend_string *key) {
	HashTable *rt = php_inspector_table_select(root, type);
	HashTable *table;

	ALLOC_HASHTABLE(table);
	zend_hash_init(table, 8, NULL, ZVAL_PTR_DTOR, 0);

	return zend_hash_update_ptr(rt, (zend_string*) key, table);
}

static zend_always_inline HashTable* php_inspector_table(php_inspector_root_t root, php_inspector_table_t type, zend_string *key, zend_bool create) {
	HashTable *rt = php_inspector_table_select(root, type);
	HashTable *table = 
		(HashTable*) zend_hash_find_ptr(rt, key);

	if (!table && create) {
		return php_inspector_table_create(root, type, key);
	}

	return table;
}

static zend_always_inline void php_inspector_table_insert(php_inspector_root_t root, php_inspector_table_t type, zend_string *key, zval *zv) {
	HashTable *table = php_inspector_table(root, type, key, 1);

	if (!table) {
		return;
	}

	zend_hash_next_index_insert(table, zv);
}

static zend_always_inline void php_inspector_table_drop(php_inspector_root_t root, php_inspector_table_t type, zend_string *key) {
	HashTable *rt = 
		php_inspector_table_select(root, type);
	
	zend_hash_del(rt, key);
}

#if defined(ZTS) && defined(COMPILE_DL_INSPECTOR)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

#endif	/* PHP_INSPECTOR_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
