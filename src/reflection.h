/*
  +----------------------------------------------------------------------+
  | inspector                                                            |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2018                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe <krakjoe@php.net>                                    |
  +----------------------------------------------------------------------+
 */
#ifndef HAVE_INSPECTOR_REFLECTION_H
#define HAVE_INSPECTOR_REFLECTION_H

extern PHP_MINIT_FUNCTION(inspector_reflection);
extern PHP_RINIT_FUNCTION(inspector_reflection);

extern zend_class_entry* php_inspector_reflection_class_ce;
extern zend_class_entry* php_inspector_reflection_method_ce;
extern zend_class_entry* php_inspector_reflection_function_ce;
extern zend_class_entry* php_inspector_reflectable_ce;

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

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(Reflectable_getReflector_arginfo, 0, 0, Reflector, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Reflectable_getReflector_arginfo, 0, 0, IS_OBJECT, "Reflector", 0)
#endif
ZEND_END_ARG_INFO()

void php_inspector_reflection_object_factory(
	zval *return_value, 
	zend_class_entry *ce, 
	php_reflection_type_t type, 
	void *ptr,
	zend_string *named);
#endif
