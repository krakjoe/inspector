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

#ifndef HAVE_INSPECTOR_REFLECTION_H
#define HAVE_INSPECTOR_REFLECTION_H

extern zend_class_entry *reflection_function_ptr;
extern zend_class_entry *reflection_method_ptr;
extern zend_class_entry *reflection_class_ptr;
extern zend_class_entry *reflection_exception_ptr;

typedef enum {
	PHP_REF_TYPE_OTHER,
	PHP_REF_TYPE_FUNCTION,
	PHP_REF_TYPE_GENERATOR,
	PHP_REF_TYPE_PARAMETER,
	PHP_REF_TYPE_TYPE,
	PHP_REF_TYPE_PROPERTY,
	PHP_REF_TYPE_DYNAMIC_PROPERTY,
	PHP_REF_TYPE_CLASS_CONSTANT
} php_reflection_type_t;

typedef struct {
	zval dummy;
	zval obj;
	void *ptr;
	zend_class_entry *ce;
	php_reflection_type_t ref_type;
	unsigned int ignore_visibility:1;
	zend_object zo;
} php_reflection_object_t;

#define php_reflection_object_from(o) \
	((php_reflection_object_t*) \
		(((char*) o) - XtOffsetOf(php_reflection_object_t, zo)))
#define php_reflection_object_fetch(z) php_reflection_object_from(Z_OBJ_P(z))

#define php_reflection_object_function(z) ((zend_function*) (php_reflection_object_fetch(z)->ptr))
#define php_reflection_object_class(z) ((zend_class_entry*) (php_reflection_object_fetch(z)->ptr))
#endif
