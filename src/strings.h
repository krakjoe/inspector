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

#ifndef HAVE_INSPECTOR_STRINGS_H
#define HAVE_INSPECTOR_STRINGS_H

typedef enum _php_inspector_string_t {
	PHP_INSPECTOR_STR_HIT = -1,
	PHP_INSPECTOR_STR_NAME = -2,
	PHP_INSPECTOR_STR_CLASS = -3,
	PHP_INSPECTOR_STR_ONRESOLVE = -4,
	PHP_INSPECTOR_STR_INSTRUCTION_CACHE = -5,
} php_inspector_string_t;

PHP_MINIT_FUNCTION(inspector_strings);
PHP_MSHUTDOWN_FUNCTION(inspector_strings);

zend_string* php_inspector_strings_fetch(php_inspector_string_t id);
void php_inspector_strings_register_opcode(zend_uchar opcode, const char *name);

#define PHP_INSPECTOR_STRING_HIT         	php_inspector_strings_fetch(PHP_INSPECTOR_STR_HIT)
#define PHP_INSPECTOR_STRING_NAME        	php_inspector_strings_fetch(PHP_INSPECTOR_STR_NAME)
#define PHP_INSPECTOR_STRING_CLASS       	php_inspector_strings_fetch(PHP_INSPECTOR_STR_CLASS)
#define PHP_INSPECTOR_STRING_ONRESOLVE  	 php_inspector_strings_fetch(PHP_INSPECTOR_STR_ONRESOLVE)
#define PHP_INSPECTOR_STRING_INSTRUCTION_CACHE   php_inspector_strings_fetch(PHP_INSPECTOR_STR_INSTRUCTION_CACHE)

#endif
