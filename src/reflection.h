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

#include "zend_exceptions.h"

#include "function.h"

extern zend_class_entry *reflector_ptr;
extern zend_class_entry *reflection_function_ptr;
extern zend_class_entry *reflection_method_ptr;
extern zend_class_entry *reflection_class_ptr;
extern zend_class_entry *reflection_exception_ptr;

static zend_always_inline zend_bool php_inspector_reflection_guard(zval *object) {
	php_inspector_function_t *function = php_inspector_function_from(object);

	if (!function->function) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"Reflector is pending");
		return 0;
	}

	if (function->expired) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"Reflector is expired");
		return 0;
	}

	return 1;
}

static zend_always_inline void php_inspector_reflector_call(zval *reflector, zend_string *method, zval *args, zval *return_value) {
	zend_fcall_info            fci = empty_fcall_info;
	zend_fcall_info_cache      fcc = empty_fcall_info_cache;
	zend_string               *key = zend_string_tolower(method);

	fci.size            = sizeof(zend_fcall_info);
	fci.object          = Z_OBJ_P(reflector);

#if PHP_VERSION_ID < 70300
	fcc.initialized     = 1;
#endif
	fcc.object          = fci.object;
	
	if ((fcc.function_handler = zend_hash_find_ptr(&Z_OBJCE_P(reflector)->function_table, key))) {
		zend_fcall_info_call(&fci, &fcc, return_value, args);
	} else {
		
	}

	zend_string_release(key);
}

/* {{{ */
ZEND_BEGIN_ARG_INFO_EX(InspectorReflector_call_arginfo, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, args,   IS_ARRAY, 0)
ZEND_END_ARG_INFO() /* }}} */
#endif
