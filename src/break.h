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

#ifndef HAVE_INSPECTOR_BREAK_H
#define HAVE_INSPECTOR_BREAK_H

extern zend_class_entry *php_inspector_break_ce;

typedef struct _php_inspector_break_t {
	zval instruction;
	zend_uchar opcode;
	struct {
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;
	} cache;
	zend_object std;
} php_inspector_break_t;

#ifndef INSPECTOR_DEBUG_BREAK
#define INSPECTOR_DEBUG_BREAK 255
#endif

#define php_inspector_break_fetch_from(o) ((php_inspector_break_t*) (((char*)o) - XtOffsetOf(php_inspector_break_t, std)))
#define php_inspector_break_fetch(z) php_inspector_break_fetch_from(Z_OBJ_P(z))
#define php_inspector_break_this() php_inspector_break_fetch(getThis())

PHP_MINIT_FUNCTION(inspector_break);
PHP_RINIT_FUNCTION(inspector_break);
PHP_RSHUTDOWN_FUNCTION(inspector_break);

void php_inspector_break_find(zval *return_value, php_inspector_instruction_t *instruction);
php_inspector_break_t* php_inspector_break_find_ptr(php_inspector_instruction_t *instruction);
zend_function *php_inspector_break_source(zend_string *file);
void php_inspector_break_pending(zend_string *file, zval *function);
int php_inspector_break_resolve(zval *zv, zend_function *ops);
#endif
