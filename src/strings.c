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

#ifndef HAVE_INSPECTOR_STRINGS
#define HAVE_INSPECTOR_STRINGS

#include "php.h"

#ifndef ZEND_VM_LAST_OPCODE
#define ZEND_VM_LAST_OPCODE ZEND_DECLARE_ANON_INHERITED_CLASS
#endif

#include "strings.h"

ZEND_BEGIN_MODULE_GLOBALS(inspector_strings)
	HashTable table;
ZEND_END_MODULE_GLOBALS(inspector_strings)

ZEND_DECLARE_MODULE_GLOBALS(inspector_strings);

typedef struct _php_inspector_string {
	zend_ulong idx;
	char *val;
	size_t len;
} php_inspector_string;

#define php_inspector_string_init(idx, literal) {idx, ZEND_STRL(literal)}

static const php_inspector_string php_inspector_strings[] = {
	php_inspector_string_init(PHP_INSPECTOR_STR_HIT, "hit"),
	php_inspector_string_init(PHP_INSPECTOR_STR_NAME, "name"),
	php_inspector_string_init(PHP_INSPECTOR_STR_CLASS, "class"),
	php_inspector_string_init(PHP_INSPECTOR_STR_ONRESOLVE, "onresolve"),
	php_inspector_string_init(PHP_INSPECTOR_STR_ONTRACE, "ontrace"),
	php_inspector_string_init(PHP_INSPECTOR_STR_INSTRUCTION_CACHE, "instructionCache"),
};

#ifdef ZTS
#define ISTR(v) TSRMG(inspector_strings_globals_id, zend_inspector_strings_globals *, v)
#else
#define ISTR(v) (inspector_strings_globals.v)
#endif

static void php_inspector_strings_init(zend_inspector_strings_globals *sg) {
	memset(sg, 0, sizeof(*sg));
}

void php_inspector_string_free(zval *zv) {
	zend_string_free(Z_STR_P(zv));
}

PHP_MINIT_FUNCTION(inspector_strings) 
{
	ZEND_INIT_MODULE_GLOBALS(inspector_strings, php_inspector_strings_init, NULL);

	zend_hash_init(&ISTR(table), 8, NULL, php_inspector_string_free, 1);

	/* negative */
	{
		const php_inspector_string *string = php_inspector_strings;
		const php_inspector_string *end    = 
			string + (sizeof(php_inspector_strings) / sizeof(php_inspector_string));

		do {
			zend_hash_index_update_ptr(&ISTR(table), string->idx, zend_string_init(string->val, string->len, 1));
		} while (++string < end);
	}

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(inspector_strings) 
{
	zend_hash_destroy(&ISTR(table));

	return SUCCESS;
}

void php_inspector_strings_register_opcode(zend_uchar opcode, const char *name) {
	if (name) {
		zend_hash_index_update_ptr(&ISTR(table), opcode, zend_string_init(name, strlen(name), 1));
	}
}

void php_inspector_strings_register_long(zend_long value, const char *name) {
	if (name) {
		zend_hash_index_update_ptr(&ISTR(table), value, zend_string_init(name, strlen(name), 1));
	}
}

zend_string* php_inspector_strings_fetch(php_inspector_string_t idx) {
	return (zend_string *) zend_hash_index_find_ptr(&ISTR(table), idx);
}
#endif
