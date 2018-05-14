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
#ifndef HAVE_INSPECTOR_BREAK
#define HAVE_INSPECTOR_BREAK

#include "php.h"

#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
#include "zend_vm.h"

#include "scope.h"
#include "entry.h"
#include "opline.h"
#include "break.h"
#include "frame.h"

ZEND_BEGIN_MODULE_GLOBALS(inspector_break)
	HashTable table;
ZEND_END_MODULE_GLOBALS(inspector_break)

ZEND_DECLARE_MODULE_GLOBALS(inspector_break);

#ifdef ZTS
#define BRK(v) TSRMG(inspector_break_globals_id, zend_inspector_break_globals *, v)
#else
#define BRK(v) (inspector_break_globals.v)
#endif

static void php_inspector_break_globals_init(zend_inspector_break_globals *BRK) {
	memset(BRK, 0, sizeof(*BRK));
}

zend_class_entry *php_inspector_break_ce;
zend_object_handlers php_inspector_break_handlers;

static zend_object* php_inspector_break_create(zend_class_entry *ce) {
	php_inspector_break_t *brk =
		(php_inspector_break_t*)
			ecalloc(1, sizeof(php_inspector_break_t) + zend_object_properties_size(ce));

	zend_object_std_init(&brk->std, ce);

	object_properties_init(&brk->std, ce);

	brk->cache.fci = empty_fcall_info;
	brk->cache.fcc = empty_fcall_info_cache;

	brk->std.handlers = &php_inspector_break_handlers;

	return &brk->std;
}

static void php_inspector_break_destroy(zend_object *zo) {
	php_inspector_break_t *brk = 
		php_inspector_break_fetch_from(zo);
	php_inspector_opline_t *opline = 
		php_inspector_opline_fetch(&brk->opline);

	zend_hash_index_del(&BRK(table), (zend_ulong) opline->opline);	

	if (Z_TYPE(brk->opline) != IS_UNDEF) {
		zval_ptr_dtor(&brk->opline);
	}

	zend_object_std_dtor(&brk->std);
}

static inline zend_bool php_inspector_break_enable(php_inspector_break_t *brk) {
	php_inspector_opline_t *opline =
		php_inspector_opline_fetch(&brk->opline);

	if (!zend_hash_index_add_ptr(&BRK(table), (zend_ulong) opline->opline, brk)) {
		return 0;
	}

	brk->opcode = opline->opline->opcode;

	opline->opline->opcode = INSPECTOR_DEBUG_BREAK;

	zend_vm_set_opcode_handler(opline->opline);

	return 1;
}

static inline zend_bool php_inspector_break_enabled(php_inspector_break_t *brk) {
	php_inspector_opline_t *opline =
		php_inspector_opline_fetch(&brk->opline);

	return zend_hash_index_exists(&BRK(table), (zend_ulong) opline->opline);
}

static inline zend_bool php_inspector_break_disable(php_inspector_break_t *brk) {
	php_inspector_opline_t *opline =
		php_inspector_opline_fetch(&brk->opline);

	return zend_hash_index_del(&BRK(table), (zend_ulong) opline->opline) == SUCCESS;
}

php_inspector_break_t* php_inspector_break_find_ptr(php_inspector_opline_t *opline) {
	return zend_hash_index_find_ptr(&BRK(table), (zend_ulong) opline->opline);;
}

void php_inspector_break_find(zval *return_value, php_inspector_opline_t *opline) {
	php_inspector_break_t *brk = php_inspector_break_find_ptr(opline);

	if (!brk) {
		return;
	}

	ZVAL_OBJ(return_value, &brk->std);
	Z_ADDREF_P(return_value);
}

/* {{{ */
static PHP_METHOD(BreakPoint, __construct)
{
	php_inspector_break_t *brk = php_inspector_break_this();
	php_inspector_opline_t *opline;
	zval *ol = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "O", &ol, php_inspector_opline_ce) != SUCCESS) {
		return;
	}

	opline = php_inspector_opline_fetch(ol);

	if (zend_hash_index_exists(&BRK(table), (zend_ulong) opline->opline)) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0, 
			"breakpoint at %p already exists", opline->opline);
		return;
	}

	ZVAL_COPY(&brk->opline, ol);

	if (!php_inspector_break_enable(brk)) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0, 
			"already a BreakPoint at that address");
	}
}

PHP_METHOD(BreakPoint, getOpcode)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();
	const char *name = zend_get_opcode_name(brk->opcode);
	
	if (!name) {
		return;
	}

	RETURN_STRING(name);
}

PHP_METHOD(BreakPoint, getOpline)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	ZVAL_COPY(return_value, &brk->opline);
}

PHP_METHOD(BreakPoint, disable)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	RETURN_BOOL(php_inspector_break_disable(brk));
}

PHP_METHOD(BreakPoint, enable)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	RETURN_BOOL(php_inspector_break_enable(brk));
}

PHP_METHOD(BreakPoint, isEnabled)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	RETURN_BOOL(php_inspector_break_enabled(brk));
}

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(BreakPoint_getOpcode_arginfo, 0, 0, IS_STRING, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(BreakPoint_getOpcode_arginfo, 0, 0, IS_STRING, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(BreakPoint_getOpline_arginfo, 0, 0, Inspector\\Opline, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(BreakPoint_getOpline_arginfo, 0, 0, IS_OBJECT, "Inspector\\Opline", 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(BreakPoint_switch_arginfo, 0, 0, _IS_BOOL, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(BreakPoint_switch_arginfo, 0, 0, _IS_BOOL, NULL, 0)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(BreakPoint_construct_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, opline, Inspector\\Opline, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(BreakPoint_hit_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, frame, Inspector\\Frame, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_break_methods[] = {
	PHP_ME(BreakPoint, __construct, BreakPoint_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(BreakPoint, getOpcode, BreakPoint_getOpcode_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(BreakPoint, getOpline, BreakPoint_getOpline_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(BreakPoint, disable, BreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(BreakPoint, enable, BreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(BreakPoint, isEnabled, BreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ABSTRACT_ME(BreakPoint, hit, BreakPoint_hit_arginfo)
	PHP_FE_END
};

static int php_inspector_break_handler(zend_execute_data *execute_data) {
	zend_op *opline = (zend_op *) EX(opline);
	php_inspector_break_t *brk = 
		zend_hash_index_find_ptr(&BRK(table), (zend_ulong) opline);

	{
		zval rv;
		zval ip;

		if (brk->cache.fci.size == 0) {
			brk->cache.fci.size = sizeof(zend_fcall_info);
			brk->cache.fci.object = &brk->std;
#if PHP_VERSION_ID < 70200
			brk->cache.fcc.initialized = 1;
#endif
			brk->cache.fcc.object = brk->cache.fci.object;
			brk->cache.fcc.function_handler = (zend_function*)
				zend_hash_str_find_ptr(
					&brk->std.ce->function_table, "hit", sizeof("hit")-1);
		}

		ZVAL_NULL(&rv);

		php_inspector_frame_construct(&ip, execute_data);

		brk->cache.fci.param_count = 1;
		brk->cache.fci.params = &ip;
		brk->cache.fci.retval = &rv;

		zend_call_function(&brk->cache.fci, &brk->cache.fcc);

		if (Z_REFCOUNTED(rv)) {
			zval_ptr_dtor(&rv);
		}

		zval_ptr_dtor(&ip);
	}

	return ZEND_USER_OPCODE_DISPATCH_TO | brk->opcode;
}

PHP_MINIT_FUNCTION(inspector_break) {
	zend_class_entry ce;

	ZEND_INIT_MODULE_GLOBALS(inspector_break, php_inspector_break_globals_init, NULL);	

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "BreakPoint", php_inspector_break_methods);
	php_inspector_break_ce = 
		zend_register_internal_class(&ce);
	php_inspector_break_ce->create_object = php_inspector_break_create;

	memcpy(&php_inspector_break_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_inspector_break_handlers.offset = XtOffsetOf(php_inspector_break_t, std);
	php_inspector_break_handlers.free_obj = php_inspector_break_destroy;

	zend_set_user_opcode_handler(INSPECTOR_DEBUG_BREAK, php_inspector_break_handler);

	return SUCCESS;
} /* }}} */

/* {{{ */
static inline void php_inspector_break_unset(zval *zv) {
	php_inspector_break_t *brk = Z_PTR_P(zv);
	php_inspector_opline_t *opline = 
		php_inspector_opline_fetch(&brk->opline);

	opline->opline->opcode = brk->opcode;

	zend_vm_set_opcode_handler(opline->opline);
} /* }}} */

/* {{{ */
PHP_RINIT_FUNCTION(inspector_break) 
{
	zend_hash_init(&BRK(table), 32, NULL, php_inspector_break_unset, 0);

	return SUCCESS;
} /* }}} */

/* {{{ */
PHP_RSHUTDOWN_FUNCTION(inspector_break)
{
	zend_hash_destroy(&BRK(table));

	return SUCCESS;
} /* }}} */
#endif
