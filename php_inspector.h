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

ZEND_BEGIN_MODULE_GLOBALS(inspector)
	struct {
		HashPosition function;
		HashPosition class;
	} pointers;
	struct {
		HashTable function;
		HashTable class;
	} pending;
	struct {
		HashTable function;
		HashTable class;
	} registered;
ZEND_END_MODULE_GLOBALS(inspector)

ZEND_DECLARE_MODULE_GLOBALS(inspector);

#ifdef ZTS
#define PIG(v) TSRMG(inspector_globals_id, zend_inspector_globals *, v)
#else
#define PIG(v) (inspector_globals.v)
#endif

static zend_always_inline HashTable* php_inspector_pending_function(zend_string *name, zend_bool create) {
	zend_string *lookup = zend_string_tolower(name);
	HashTable *pending = zend_hash_find_ptr(&PIG(pending).function, lookup);
	
	if (!pending && create) {
		ALLOC_HASHTABLE(pending);

		zend_hash_init(pending, 8, NULL, ZVAL_PTR_DTOR, 0);

		zend_hash_update_ptr(&PIG(pending).function, lookup, pending);
	}

	zend_string_release(lookup);

	return pending;
}

static zend_always_inline HashTable* php_inspector_registered_function(zend_string *name, zend_bool create) {
	zend_string *lookup = zend_string_tolower(name);
	HashTable *registered = zend_hash_find_ptr(&PIG(registered).function, lookup);
	
	if (!registered && create) {
		ALLOC_HASHTABLE(registered);

		zend_hash_init(registered, 8, NULL, ZVAL_PTR_DTOR, 0);

		zend_hash_update_ptr(&PIG(registered).function, lookup, registered);
	}

	zend_string_release(lookup);

	return registered;
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
