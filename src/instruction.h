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

#ifndef HAVE_INSPECTOR_INSTRUCTION_H
#define HAVE_INSPECTOR_INSTRUCTION_H
typedef struct _php_inspector_instruction_t {
	zend_op *opline;
	zval function;
	zval previous;
	zval next;
	zend_object std;
} php_inspector_instruction_t;

extern zend_class_entry *php_inspector_instruction_ce;
extern zend_class_entry *php_inspector_instruction_interface_ce;

#define php_inspector_instruction_fetch_from(o) ((php_inspector_instruction_t*) (((char*)o) - XtOffsetOf(php_inspector_instruction_t, std)))
#define php_inspector_instruction_fetch(z) php_inspector_instruction_fetch_from(Z_OBJ_P(z))
#define php_inspector_instruction_this() php_inspector_instruction_fetch(getThis())

void php_inspector_instruction_factory(zval *function, zend_op *instruction, zval *return_value);
void php_inspector_instruction_cache_flush(zval *function, zval *return_value);

PHP_MINIT_FUNCTION(inspector_instruction);
PHP_MINIT_FUNCTION(inspector_instruction_interface);
#endif
