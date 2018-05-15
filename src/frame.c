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
#ifndef HAVE_INSPECTOR_FRAME
#define HAVE_INSPECTOR_FRAME

#include "php.h"

#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
#include "zend_vm.h"

#include "scope.h"
#include "closure.h"
#include "method.h"
#include "func.h"
#include "entry.h"
#include "opline.h"
#include "frame.h"

zend_class_entry *php_inspector_frame_ce;
zend_object_handlers php_inspector_frame_handlers;

static zend_object* php_inspector_frame_create(zend_class_entry *ce) {
	php_inspector_frame_t *frame =
		(php_inspector_frame_t*)
			ecalloc(1, sizeof(php_inspector_frame_t) + zend_object_properties_size(ce));

	zend_object_std_init(&frame->std, ce);

	object_properties_init(&frame->std, ce);

	frame->std.handlers = &php_inspector_frame_handlers;

	return &frame->std;
}

static void php_inspector_frame_free(zend_object *zo) {
	php_inspector_frame_t *frame =
		php_inspector_frame_fetch_from(zo);

	if (Z_TYPE(frame->scope) != IS_UNDEF)
		zval_ptr_dtor(&frame->scope);

	if (Z_TYPE(frame->opline) != IS_UNDEF)
		zval_ptr_dtor(&frame->opline);

	if (frame->op1.f) {
		zval_ptr_dtor(frame->op1.f);
	}

	if (frame->op2.f) {
		zval_ptr_dtor(frame->op2.f);
	}

	zend_object_std_dtor(&frame->std);
}

void php_inspector_frame_construct(zval *zv, zend_execute_data *execute_data) {
	php_inspector_frame_t *frame;

	object_init_ex(zv, php_inspector_frame_ce);

	frame = php_inspector_frame_fetch(zv);
	frame->frame = execute_data;

	if (frame->frame->func->common.fn_flags & ZEND_ACC_CLOSURE) {
		object_init_ex(&frame->scope, php_inspector_closure_ce);
	} else if (frame->frame->func->common.scope) {
		object_init_ex(&frame->scope, php_inspector_method_ce);
	} else {
		object_init_ex(&frame->scope, php_inspector_func_ce);
	}

	php_inspector_scope_construct(
		&frame->scope, frame->frame->func);
	php_inspector_opline_construct(
		&frame->opline, 
		&frame->scope, (zend_op*) frame->frame->opline);

	if (frame->frame->opline->op1_type != IS_UNUSED) {
		frame->op1.p = zend_get_zval_ptr(
#if PHP_VERSION_ID >= 70300
			frame->frame->opline, 
#endif
			frame->frame->opline->op1_type, 
			&frame->frame->opline->op1, 
			frame->frame, &frame->op1.f, BP_VAR_RW);
	}

	if (frame->frame->opline->op2_type != IS_UNUSED) {
		frame->op2.p = zend_get_zval_ptr(
#if PHP_VERSION_ID >= 70300
			frame->frame->opline, 
#endif
			frame->frame->opline->op2_type, 
			&frame->frame->opline->op2, 
			frame->frame, &frame->op2.f, BP_VAR_RW);
	}
}

PHP_METHOD(Frame, getOpline)
{
	php_inspector_frame_t *frame = 
		php_inspector_frame_this();
	
	ZVAL_COPY(return_value, &frame->opline);
}

PHP_METHOD(Frame, getStack)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();
	php_inspector_scope_t *scope =
		php_inspector_scope_fetch(&frame->scope);

	zval *var = ZEND_CALL_VAR_NUM(frame->frame, 0),
	     *end = var + scope->ops->last_var;

	array_init_size(return_value, scope->ops->last_var);

	while (var < end) {
		zval *val = var;

		if (Z_TYPE_P(val) == IS_UNDEF) {
			var++;
			continue;
		}

		if (Z_TYPE_P(val) == IS_INDIRECT) {
			val = Z_INDIRECT_P(val);
		}

		zend_hash_add(Z_ARRVAL_P(return_value), 
			scope->ops->vars[scope->ops->last_var - (end - var)],
			val);
		Z_TRY_ADDREF_P(val);

		var++;
	}
}

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(Frame_getOpline_arginfo, 0, 0, Inspector\\Opline, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Frame_getOpline_arginfo, 0, 0, IS_OBJECT, "Inspector\\Opline", 0)
#endif
ZEND_END_ARG_INFO()

PHP_METHOD(Frame, getScope)
{
	php_inspector_frame_t *frame = 
		php_inspector_frame_this();

	ZVAL_COPY(return_value, &frame->scope);
}

PHP_METHOD(Frame, getSymbols)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();

	if (frame->frame->symbol_table) {
#if PHP_VERSION_ID >= 70300
		GC_ADDREF(frame->frame->symbol_table);
#else
		GC_REFCOUNT(frame->frame->symbol_table)++;
#endif

		RETURN_ARR(frame->frame->symbol_table);
	}
}

PHP_METHOD(Frame, getPrevious)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();

	if (frame->frame->prev_execute_data) {
		php_inspector_frame_construct(
			return_value, 
			frame->frame->prev_execute_data);
	}
}

PHP_METHOD(Frame, getOperand)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();
	zend_long which = PHP_INSPECTOR_OPLINE_INVALID;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l", &which) != SUCCESS) {
		return;
	}

	switch (which) {
		case PHP_INSPECTOR_OPLINE_OP1:
			if (frame->op1.p)
				ZVAL_COPY(return_value, frame->op1.p);
		break;

		case PHP_INSPECTOR_OPLINE_OP2:
			if (frame->op2.p)
				ZVAL_COPY(return_value, frame->op2.p);
		break;
	}
}

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(Frame_getScope_arginfo, 0, 0, Inspector\\Scope, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Frame_getScope_arginfo, 0, 0, IS_OBJECT, "Inspector\\Scope", 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Frame_getSymbols_arginfo, 0, 0, IS_ARRAY, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Frame_getSymbols_arginfo, 0, 0, IS_ARRAY, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(Frame_getPrevious_arginfo, 0, 0, Inspector\\Frame, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Frame_getPrevious_arginfo, 0, 0, IS_OBJECT, "Inspector\\Frame", 1)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Frame_getOperand_arginfo, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Frame_getStack_arginfo, 0, 0, IS_ARRAY, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Frame_getStack_arginfo, 0, 0, IS_ARRAY, NULL, 1)
#endif
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_frame_methods[] = {
	PHP_ME(Frame, getOpline, Frame_getOpline_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Frame, getScope, Frame_getScope_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Frame, getSymbols, Frame_getSymbols_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Frame, getPrevious, Frame_getPrevious_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Frame, getOperand, Frame_getOperand_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Frame, getStack, Frame_getStack_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_frame) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Frame", php_inspector_frame_methods);
	php_inspector_frame_ce = 
		zend_register_internal_class(&ce);
	php_inspector_frame_ce->ce_flags |= ZEND_ACC_FINAL;
	php_inspector_frame_ce->create_object = php_inspector_frame_create;

	memcpy(&php_inspector_frame_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_inspector_frame_handlers.offset = XtOffsetOf(php_inspector_frame_t, std);
	php_inspector_frame_handlers.free_obj = php_inspector_frame_free;

	return SUCCESS;
} /* }}} */
#endif
