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

#ifndef HAVE_INSPECTOR_MAP
#define HAVE_INSPECTOR_MAP

#include "php.h"
#include "php_inspector.h"

#include "zend_extensions.h"

ZEND_BEGIN_MODULE_GLOBALS(inspector_map)
	HashTable table;

	zend_op_array *map;
	zend_op_array *src;
ZEND_END_MODULE_GLOBALS(inspector_map);

ZEND_DECLARE_MODULE_GLOBALS(inspector_map);

#ifdef ZTS
#define IMG(v) TSRMG(inspector_map_globals_id, zend_inspector_map_globals *, v)
#else
#define IMG(v) (inspector_map_globals.v)
#endif

#ifndef GC_ADDREF
#	define GC_ADDREF(g) ++GC_REFCOUNT(g)
#	define GC_DELREF(g) --GC_REFCOUNT(g)
#endif

static int php_inspector_map_function_id;

#define php_inspector_map_function(function) (function)->reserved[php_inspector_map_function_id]

typedef void (*php_inspector_map_callback_t)(void *);

static zend_always_inline void* php_inspector_map_dup(void *ptr, size_t num, size_t size, php_inspector_map_callback_t constructor) {
	void *dup;

	if (!num) {
		return NULL;
	}

	dup = (void*) ecalloc(num, size);

	memcpy(dup, ptr, num * size);

	if (constructor) {
		char *begin = (char*) dup,
		     *end   = ((char*) begin) + (num * size);

		while (begin < end) {
			constructor(
				(void*) begin);
			begin += size;
		}
	}

	return dup;
}

static zend_always_inline void php_inspector_map_apply(void *ptr, size_t num, size_t size, php_inspector_map_callback_t constructor) {
	if (!num) {
		return;
	}

	if (constructor) {
		char *begin = (char*) ptr,
		     *end   = ((char*) begin) + (num * size);

		while (begin < end) {
			constructor(
				(void*) begin);
			begin += size;
		}
	}
}

static zend_always_inline zend_string** php_inspector_map_dup_strings(zend_string **strings, size_t size) {
	zend_string **dup = 
		(zend_string**) 
			ecalloc(size, sizeof(zend_string*));
	size_t string = 0;

	memcpy(dup, strings, sizeof(zend_string*) * size);	

	for (string = 0; string < size; string++) {
		zend_string_addref(dup[string]);
	}

	return dup;	
}

static zend_always_inline void php_inspector_map_free(void *ptr, size_t num, size_t size, php_inspector_map_callback_t destructor) {
	if (destructor) {
		php_inspector_map_apply(ptr, num, size, destructor);
	}

	efree(ptr);
}
static zend_always_inline void php_inspector_map_reindex_const_op(zend_op_array *op_array, zend_op *orig_opcodes, zval *orig_literals) {
	zend_op *opline = op_array->opcodes, *end = opline + op_array->last;

	for (; opline < end; opline++) {
#if ZEND_USE_ABS_CONST_ADDR
		if (opline->op1_type == IS_CONST) {
			opline->op1.zv = (zval*)((char*)opline->op1.zv + ((char*)op_array->literals - (char*)orig_literals));
		}
		if (opline->op2_type == IS_CONST) {
			opline->op2.zv = (zval*)((char*)opline->op2.zv + ((char*)op_array->literals - (char*)orig_literals));
		}
#else
		if (opline->op1_type == IS_CONST) {
			opline->op1.constant =
				(char*)(op_array->literals +
					((zval*)((char*)(orig_opcodes + (opline - op_array->opcodes)) +
					(int32_t)opline->op1.constant) - orig_literals)) -
				(char*)opline;
		}
		if (opline->op2_type == IS_CONST) {
			opline->op2.constant =
				(char*)(op_array->literals +
					((zval*)((char*)(orig_opcodes + (opline - op_array->opcodes)) +
					(int32_t)opline->op2.constant) - orig_literals)) -
				(char*)opline;
		}
#endif
	}
}

static zend_always_inline void php_inspector_map_opcode(zend_op *opline) {
	zend_op *old = 
		IMG(src)->opcodes + (opline - IMG(map)->opcodes);

#if 0
	php_printf("%p %p -> %p %p\n", IMG(src)->opcodes, old, IMG(map)->opcodes, opline);
#endif

#if ZEND_USE_ABS_CONST_ADDR
	
#endif

	switch (opline->opcode) {
		case ZEND_JMP:
		case ZEND_FAST_CALL:
			ZEND_PASS_TWO_UNDO_JMP_TARGET(IMG(src), old, opline->op1);
			ZEND_PASS_TWO_UPDATE_JMP_TARGET(IMG(map), opline, opline->op1);
			break;

		case ZEND_JMPZNZ:
		case ZEND_JMPZ:
		case ZEND_JMPNZ:
		case ZEND_JMPZ_EX:
		case ZEND_JMPNZ_EX:
		case ZEND_JMP_SET:
		case ZEND_COALESCE:
		case ZEND_NEW:
		case ZEND_FE_RESET_R:
		case ZEND_FE_RESET_RW:
		case ZEND_ASSERT_CHECK:
			ZEND_PASS_TWO_UNDO_JMP_TARGET(IMG(src), old, opline->op2);
			ZEND_PASS_TWO_UPDATE_JMP_TARGET(IMG(map), opline, opline->op2);
			break;

		case ZEND_FE_FETCH_R:
		case ZEND_FE_FETCH_RW:
#if PHP_VERSION_ID < 70300
		case ZEND_DECLARE_ANON_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
#endif
			opline->extended_value = 
				ZEND_OPLINE_NUM_TO_OFFSET(
					IMG(map), opline, ZEND_OFFSET_TO_OPLINE_NUM(IMG(map), old, old->extended_value));
		break;

#if PHP_VERSION_ID >= 70300
		case ZEND_CATCH:
			if (!(opline->extended_value & ZEND_LAST_CATCH)) {
				ZEND_PASS_TWO_UNDO_JMP_TARGET(IMG(src), old, opline->op2);
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(IMG(map), opline, opline->op2);
			}
		break;
#endif
	}
}

static zend_always_inline void php_inspector_map_arginfo_addref(zend_arg_info *info) {
	if (info->name) {
		zend_string_addref(info->name);
	}

#if PHP_VERSION_ID <= 70200
	if (info->class_name) {
		zend_string_addref(info->class_name);
	}
#else
	if (ZEND_TYPE_IS_SET(info->type) && ZEND_TYPE_IS_CLASS(info->type)) {
		zend_string_addref(ZEND_TYPE_NAME(info->type));
	}
#endif
}

static zend_always_inline void php_inspector_map_arginfo_delref(zend_arg_info *info) {
	if (info->name) {
		zend_string_release(info->name);
	}

#if PHP_VERSION_ID <= 70200
	if (info->class_name) {
		zend_string_release(info->class_name);
	}
#else
	if (ZEND_TYPE_IS_SET(info->type) && ZEND_TYPE_IS_CLASS(info->type)) {
		zend_string_release(ZEND_TYPE_NAME(info->type));
	}
#endif
}

static zend_always_inline void php_inspector_map_string_addref(zend_string *zs) {
	zend_string_addref(zs);
}

static zend_always_inline void php_inspector_map_string_delref(zend_string *zs) {
	zend_string_release(zs);
}

static zend_always_inline void php_inspector_map_zval_addref(zval *zv) {
	Z_TRY_ADDREF_P(zv);
}

static zend_always_inline void php_inspector_map_zval_delref(zval *zv) {
	if (Z_REFCOUNTED_P(zv)) {
		zval_ptr_dtor(zv);
	}
}

static void php_inspector_map_destruct(zend_op_array *mapped) {
	if (mapped->filename) {
		zend_string_release(mapped->filename);
	}

	if (mapped->function_name) {
		zend_string_release(mapped->function_name);
	}

	if (mapped->doc_comment) {
		zend_string_release(mapped->doc_comment);
	}
	
	if (mapped->try_catch_array) {
		efree(mapped->try_catch_array);
	}

	if (mapped->live_range) {
		efree(mapped->live_range);
	}

	if (mapped->vars) {
		php_inspector_map_free(mapped->vars, 
			mapped->last_var, sizeof(zend_string*),
			(php_inspector_map_callback_t) php_inspector_map_string_delref);
	}

	if (mapped->literals) {
		php_inspector_map_free(mapped->literals,
			mapped->last_literal, sizeof(zval),
			(php_inspector_map_callback_t) php_inspector_map_zval_delref);
	}
	
	if (mapped->static_variables) {
		if (GC_DELREF(mapped->static_variables) == 0) {
			zend_array_destroy(mapped->static_variables);
		}
	}

	if (mapped->arg_info) {
		zend_arg_info *info = mapped->arg_info;
		uint32_t       end = mapped->num_args;

		if (mapped->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
			info--;
			end++;
		}

		if (mapped->fn_flags & ZEND_ACC_VARIADIC) {
			end++;
		}

		php_inspector_map_free(info, 
			end, sizeof(zend_arg_info), 
			(php_inspector_map_callback_t) php_inspector_map_arginfo_delref);
	}

	efree(mapped->run_time_cache);
	efree(mapped->opcodes);
	efree(mapped->refcount);
}

static zend_always_inline void php_inspector_map_construct(zend_op_array *mapped) {
#if PHP_VERSION_ID >= 70300
	zval *old_literals = mapped->literals;
	zend_op *old_opcodes = mapped->opcodes;
#endif

	IMG(map) = mapped;

	mapped->fn_flags &= ~ZEND_ACC_ARENA_ALLOCATED;

	mapped->refcount = (uint32_t*) emalloc(sizeof(uint32_t));

	(*mapped->refcount) = 2; /* just in case the engine gets ahold of it, and wants to free it */

	if (mapped->filename)
		zend_string_addref(mapped->filename);

	if (mapped->function_name)
		zend_string_addref(mapped->function_name);

	if (mapped->doc_comment)
		zend_string_addref(mapped->doc_comment);

	mapped->try_catch_array = (zend_try_catch_element*) php_inspector_map_dup(
		mapped->try_catch_array, mapped->last_try_catch, sizeof(zend_try_catch_element), NULL);

	mapped->live_range = (zend_live_range*) php_inspector_map_dup(
		mapped->live_range, mapped->last_live_range, sizeof(zend_live_range), NULL);

	mapped->vars    = (zend_string**) php_inspector_map_dup_strings(mapped->vars, mapped->last_var);

	mapped->literals = (zval*) php_inspector_map_dup(
		mapped->literals, mapped->last_literal, sizeof(zval),
		(php_inspector_map_callback_t) php_inspector_map_zval_addref);

	mapped->opcodes = (zend_op *) php_inspector_map_dup(
		mapped->opcodes, mapped->last, sizeof(zend_op),
		NULL);

#if PHP_VERSION_ID >= 70300
	php_inspector_map_reindex_const_op(mapped, old_opcodes, old_literals);
#endif

	php_inspector_map_apply(
		mapped->opcodes, mapped->last, sizeof(zend_op), 
		(php_inspector_map_callback_t) php_inspector_map_opcode);

	if (mapped->static_variables) {
		mapped->static_variables = zend_array_dup(mapped->static_variables);
	}

	if (mapped->num_args) {

		zend_arg_info *info = mapped->arg_info;
		uint32_t       end = mapped->num_args;

		if (mapped->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
			info--;
			end++;
		}

		if (mapped->fn_flags & ZEND_ACC_VARIADIC) {
			end++;
		}

		mapped->arg_info = php_inspector_map_dup(
			info, end, sizeof(zend_arg_info), 
			(php_inspector_map_callback_t) php_inspector_map_arginfo_addref);

		if (mapped->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
			mapped->arg_info++;
		}
	}

	mapped->run_time_cache = emalloc(mapped->cache_size);

	memset(mapped->run_time_cache, 0, mapped->cache_size);

	zend_hash_index_update_ptr(
		&IMG(table), (zend_ulong) IMG(src), IMG(map));

	php_inspector_map_function(IMG(src)) = IMG(map);

	IMG(map) = NULL;
}

zend_op_array* php_inspector_map_fetch(zend_op_array *src) {
	return php_inspector_map_function(src);
}

zend_op_array* php_inspector_map_create(zend_op_array *src) {
	zend_op_array *mapped;

	if (!ZEND_USER_CODE(src->type)) {
		return src;
	}

	if ((mapped = php_inspector_map_fetch(src))) {
		return mapped;
	}

	IMG(src) = src;

	mapped = (zend_op_array*) php_inspector_map_dup(
		src, 1, sizeof(zend_op_array), 
		(php_inspector_map_callback_t) php_inspector_map_construct);

	IMG(src) = NULL;

	return mapped;
}

void php_inspector_map_destroy(zend_op_array *map) {
	if (!ZEND_USER_CODE(map->type)) {
		return;
	}

	php_inspector_map_free(map, 
		1, sizeof(zend_op_array), 
		(php_inspector_map_callback_t) php_inspector_map_destruct);
}

static void php_inspector_map_globals(zend_inspector_map_globals *IMG) {
	memset(IMG, 0, sizeof(*IMG));
}

PHP_MINIT_FUNCTION(inspector_map)
{
	zend_extension dummy;

	ZEND_INIT_MODULE_GLOBALS(inspector_map, php_inspector_map_globals, NULL);

	php_inspector_map_function_id = zend_get_resource_handle(&dummy);

	if (php_inspector_map_function_id < 0) {
		return FAILURE;
	}

	return SUCCESS;
}

static void php_inspector_map_table_free(zval *zv) {
	php_inspector_map_destroy(Z_PTR_P(zv));
}

PHP_RINIT_FUNCTION(inspector_map)
{
	zend_hash_init(&IMG(table), 8, NULL, php_inspector_map_table_free, 0);

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(inspector_map)
{
	zend_hash_destroy(&IMG(table));

	return SUCCESS;
}
#endif
