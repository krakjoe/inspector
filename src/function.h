/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
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

/* $Id$ */
#ifndef HAVE_INSPECTOR_FUNCTION_H
#define HAVE_INSPECTOR_FUNCTION_H

extern zend_class_entry *php_inspector_function_ce;

void php_inspector_function_factory(zend_function *function, zval *return_value);

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFunction_getInstruction_arginfo, 0, 0, Inspector\\InspectorInstruction, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFunction_getInstruction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstruction", 1)
#endif
	ZEND_ARG_TYPE_INFO(0, num, IS_LONG, 0)
ZEND_END_ARG_INFO()

extern PHP_METHOD(InspectorFunction, getInstruction);

extern PHP_MINIT_FUNCTION(inspector_function);
#endif
