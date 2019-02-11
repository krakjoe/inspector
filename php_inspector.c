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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_closures.h"
#include "zend_vm.h"

#include "ext/standard/info.h"

#include "php_inspector.h"
#include "src/strings.h"
#include "src/file.h"
#include "src/class.h"
#include "src/method.h"
#include "src/function.h"
#include "src/instruction.h"
#include "src/operand.h"

#include "src/break.h"
#include "src/frame.h"

#include "src/map.h"

static void (*zend_execute_function)(zend_execute_data *);

typedef struct _php_inspector_tables_t {
	HashTable file;
	HashTable function;
	HashTable class;
} php_inspector_tables_t;

ZEND_BEGIN_MODULE_GLOBALS(inspector)
	php_inspector_tables_t pending;
	php_inspector_tables_t registered;
ZEND_END_MODULE_GLOBALS(inspector)

ZEND_DECLARE_MODULE_GLOBALS(inspector);

#ifdef ZTS
#define PIG(v) TSRMG(inspector_globals_id, zend_inspector_globals *, v)
#else
#define PIG(v) (inspector_globals.v)
#endif

static void php_inspector_globals_init(zend_inspector_globals *PIG) {
	memset(PIG, 0, sizeof(*PIG));
}

static zend_always_inline HashTable* php_inspector_table_select(php_inspector_root_t root, php_inspector_table_t type) {
	php_inspector_tables_t *tables;

	switch (root) {
		case PHP_INSPECTOR_ROOT_PENDING:
			tables = &PIG(pending);
		break;

		case PHP_INSPECTOR_ROOT_REGISTERED:
			tables = &PIG(registered);
		break;

		default:
			return NULL;
	}

	switch (type) {
		case PHP_INSPECTOR_TABLE_FILE:
			return &tables->file;
		case PHP_INSPECTOR_TABLE_FUNCTION:
			return &tables->function;
		case PHP_INSPECTOR_TABLE_CLASS:
			return &tables->class;
	}

	return NULL;
}

static zend_always_inline HashTable* php_inspector_table_create(php_inspector_root_t root, php_inspector_table_t type, 	zend_string *key) {
	HashTable *rt = php_inspector_table_select(root, type);
	HashTable *table;

	ALLOC_HASHTABLE(table);
	zend_hash_init(table, 8, NULL, ZVAL_PTR_DTOR, 0);

	zend_hash_update_ptr(rt, key, table);

	return table;
}

HashTable* php_inspector_table(php_inspector_root_t root, php_inspector_table_t type, zend_string *key, zend_bool create) {
	HashTable *rt = php_inspector_table_select(root, type);
	zend_string *name = type != PHP_INSPECTOR_TABLE_FILE ?
		zend_string_tolower(key) : zend_string_copy(key);
	HashTable *table = 
		(HashTable*) zend_hash_find_ptr(rt, name);

	if (!table && create) {
		table = php_inspector_table_create(root, type, name);
	}

	zend_string_release(name);
	return table;
}

void php_inspector_table_insert(php_inspector_root_t root, php_inspector_table_t type, zend_string *key, zval *zv) {
	HashTable *table = php_inspector_table(root, type, key, 1);

	if (!table) {
		return;
	}

	if (zend_hash_index_update(table, (zend_ulong) Z_OBJ_P(zv), zv)) {
		Z_ADDREF_P(zv);
	}
}

void php_inspector_table_drop(php_inspector_root_t root, php_inspector_table_t type, zend_string *key) {
	HashTable *rt = 
		php_inspector_table_select(root, type);
	zend_string *keyed = type != PHP_INSPECTOR_TABLE_FILE ?
			zend_string_tolower(key) : zend_string_copy(key);

	zend_hash_del(rt, keyed);
	zend_string_release(keyed);
}

static zend_always_inline void php_inspector_table_apply_specific(php_inspector_root_t root, php_inspector_table_t type, zend_string *name, apply_func_arg_t applicator, void *pointer) {
	HashTable *specific = php_inspector_table(root, type, name, 0);

	if (EXPECTED(!specific || !zend_hash_num_elements(specific))) {
		return;
	}

#if PHP_VERSION_ID >= 70300
	if (!GC_IS_RECURSIVE(specific)) {
#else
	if (ZEND_HASH_GET_APPLY_COUNT(specific) == 0) {
#endif

#if PHP_VERSION_ID >= 70300
		GC_PROTECT_RECURSION(specific);
#else
		ZEND_HASH_INC_APPLY_COUNT(specific);
#endif

		zend_hash_apply_with_argument(
			specific, applicator, pointer);

#if PHP_VERSION_ID >= 70300
		GC_UNPROTECT_RECURSION(specific);
#else
		ZEND_HASH_DEC_APPLY_COUNT(specific);
#endif

		php_inspector_table_drop(root, type, name);
	}
}

static zend_always_inline void php_inspector_table_apply(php_inspector_root_t root, php_inspector_table_t type, HashTable *zend, apply_func_arg_t applicator) {
	zend_string *name;
	HashTable   *selected = php_inspector_table_select(root, type);

	if (EXPECTED(!zend_hash_num_elements(selected))) {
		return;
	}

	ZEND_HASH_FOREACH_STR_KEY(selected, name) {
		void *pointer = zend_hash_find_ptr(zend, name);

		if (!pointer) {
			continue;
		}

		php_inspector_table_apply_specific(root, type, name, applicator, pointer);
	} ZEND_HASH_FOREACH_END();
}

static void php_inspector_trace(zend_object *tracer, zend_execute_data *execute_data) {
	int zrc = 0;

	while (1) {
		zval frame, trv;
		zend_fcall_info fci = empty_fcall_info;
		zend_fcall_info_cache fcc = empty_fcall_info_cache;

		ZVAL_UNDEF(&trv);

		fci.size = sizeof(zend_fcall_info);
		fci.param_count = 1;
		fci.params = &frame;
		fcc.object = tracer;
		fci.retval = &trv;
#if PHP_VERSION_ID < 70300
		fcc.initialized = 1;
#endif
		fcc.object      = tracer;
		fcc.function_handler = zend_hash_find_ptr(
			&tracer->ce->function_table, PHP_INSPECTOR_STRING_ONTRACE);;

		php_inspector_frame_factory(execute_data, &frame);

		if (zend_call_function(&fci, &fcc) == SUCCESS) {
			if (Z_REFCOUNTED(trv)) {
				zval_ptr_dtor(&trv);
			}
		}

		zval_ptr_dtor(&frame);

		zrc = zend_vm_call_opcode_handler(execute_data);

		if (zrc != SUCCESS) {
			if (zrc < SUCCESS) {
				return;
			}

			execute_data = EG(current_execute_data);
		}
	}
}

static void php_inspector_execute(zend_execute_data *execute_data) {
	zend_op_array *function = (zend_op_array*) EX(func);
	zend_op_array *map;

	php_inspector_table_apply(
		PHP_INSPECTOR_ROOT_PENDING,
		PHP_INSPECTOR_TABLE_CLASS,
		EG(class_table),
		(apply_func_arg_t) php_inspector_class_resolve);

	php_inspector_table_apply(
		PHP_INSPECTOR_ROOT_PENDING,
		PHP_INSPECTOR_TABLE_FUNCTION,
		EG(function_table),
		(apply_func_arg_t) php_inspector_function_resolve);

	map = php_inspector_map_fetch(function);

	if (UNEXPECTED(map)) {
		if (UNEXPECTED(!function->function_name)) {

			php_inspector_table_apply_specific(
				PHP_INSPECTOR_ROOT_PENDING,
				PHP_INSPECTOR_TABLE_FILE,
				function->filename,
				(apply_func_arg_t) php_inspector_file_resolve, map);
		}

		EX(func) = (zend_function*) map;
		EX(opline) = EX(func)->op_array.opcodes + 
				(EX(opline) - function->opcodes);

#if ZEND_EX_USE_RUN_TIME_CACHE
# if PHP_VERSION_ID >= 70400
		EX_LOAD_RUN_TIME_CACHE(map);
# else
		if (map->run_time_cache)
			EX(run_time_cache) = map->run_time_cache;
# endif
#endif

#if ZEND_EX_USE_LITERALS
		EX(literals) = map->literals;
#endif

#ifdef ZEND_DEL_CALL_FLAG
		ZEND_DEL_CALL_FLAG(execute_data, ZEND_CALL_CLOSURE);
#endif
	}

	if (php_inspector_trace_fetch(EX(func))) {
		php_inspector_trace(
			php_inspector_trace_fetch(EX(func)), 
			execute_data);
	} else zend_execute_function(execute_data);

	if (UNEXPECTED(EG(exception))) {
		php_inspector_break_handle_exception(execute_data);
	}

	if (UNEXPECTED(map)) {
		EX(func) = (zend_function*) function;
		EX(opline) = EX(func)->op_array.opcodes + 
				(EX(opline) - map->opcodes);

#if ZEND_EX_USE_RUN_TIME_CACHE
# if PHP_VERSION_ID >= 70400
		EX_LOAD_RUN_TIME_CACHE(function);
# else
		if (map->run_time_cache)
			EX(run_time_cache) = function->run_time_cache;
# endif
#endif

#if ZEND_EX_USE_LITERALS
		EX(literals) = function->literals;
#endif
	}
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(inspector)
{
	ZEND_INIT_MODULE_GLOBALS(inspector, php_inspector_globals_init, NULL);

	PHP_MINIT(inspector_instruction_interface)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_strings)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_class)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_function)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_method)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_file)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_instruction)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_operand)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_frame)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_map)(INIT_FUNC_ARGS_PASSTHRU);

	REGISTER_NS_STRING_CONSTANT("Inspector", "Version", PHP_INSPECTOR_VERSION, CONST_CS | CONST_PERSISTENT);

#define REGISTER_INSPECTOR_LONG(l) REGISTER_NS_LONG_CONSTANT("Inspector", #l, l, CONST_CS | CONST_PERSISTENT)
	REGISTER_INSPECTOR_LONG(IS_UNDEF);
	REGISTER_INSPECTOR_LONG(IS_NULL);
	REGISTER_INSPECTOR_LONG(IS_FALSE);
	REGISTER_INSPECTOR_LONG(IS_TRUE);
	REGISTER_INSPECTOR_LONG(_IS_BOOL);
	REGISTER_INSPECTOR_LONG(IS_LONG);
	REGISTER_INSPECTOR_LONG(IS_DOUBLE);
	REGISTER_INSPECTOR_LONG(IS_STRING);
	REGISTER_INSPECTOR_LONG(IS_ARRAY);
	REGISTER_INSPECTOR_LONG(IS_OBJECT);
	REGISTER_INSPECTOR_LONG(IS_RESOURCE);
	REGISTER_INSPECTOR_LONG(IS_REFERENCE);
#ifdef IS_CONSTANT
	REGISTER_INSPECTOR_LONG(IS_CONSTANT);
#endif
	REGISTER_INSPECTOR_LONG(IS_CONSTANT_AST);
	REGISTER_INSPECTOR_LONG(IS_CALLABLE);
	REGISTER_INSPECTOR_LONG(IS_ITERABLE);
	REGISTER_INSPECTOR_LONG(IS_VOID);
	REGISTER_INSPECTOR_LONG(IS_INDIRECT);
	REGISTER_INSPECTOR_LONG(IS_PTR);
	REGISTER_INSPECTOR_LONG(_IS_ERROR);

	REGISTER_INSPECTOR_LONG(IS_CONSTANT_UNQUALIFIED);
#ifdef IS_CONSTANT_VISITED_MARK
	REGISTER_INSPECTOR_LONG(IS_CONSTANT_VISITED_MARK);
#endif
	REGISTER_INSPECTOR_LONG(IS_CONSTANT_CLASS);
	REGISTER_INSPECTOR_LONG(IS_CONSTANT_IN_NAMESPACE);

	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_DEFAULT);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_SELF);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_PARENT);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_STATIC);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_AUTO);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_INTERFACE);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_TRAIT);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_MASK);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_NO_AUTOLOAD);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_SILENT);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_CLASS_EXCEPTION);

	REGISTER_INSPECTOR_LONG(ZEND_EVAL);
	REGISTER_INSPECTOR_LONG(ZEND_INCLUDE);
	REGISTER_INSPECTOR_LONG(ZEND_INCLUDE_ONCE);
	REGISTER_INSPECTOR_LONG(ZEND_REQUIRE);
	REGISTER_INSPECTOR_LONG(ZEND_REQUIRE_ONCE);

#ifdef ZEND_FETCH_STANDARD
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_STANDARD);
#endif
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_GLOBAL);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_LOCAL);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_GLOBAL_LOCK);
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_TYPE_MASK);
#ifdef ZEND_FETCH_ARG_MASK
	REGISTER_INSPECTOR_LONG(ZEND_FETCH_ARG_MASK);
#endif

#ifdef ZEND_ISSET
	REGISTER_INSPECTOR_LONG(ZEND_ISSET);
#endif
	REGISTER_INSPECTOR_LONG(ZEND_ISEMPTY);
#ifdef ZEND_ISSET_ISEMPTY_MASK
	REGISTER_INSPECTOR_LONG(ZEND_ISSET_ISEMPTY_MASK);
#endif
#ifdef ZEND_QUICK_SET
	REGISTER_INSPECTOR_LONG(ZEND_QUICK_SET);
#endif
	REGISTER_INSPECTOR_LONG(ZEND_FREE_ON_RETURN);

	REGISTER_INSPECTOR_LONG(ZEND_SEND_BY_VAL);
	REGISTER_INSPECTOR_LONG(ZEND_SEND_BY_REF);
	REGISTER_INSPECTOR_LONG(ZEND_SEND_PREFER_REF);
	REGISTER_INSPECTOR_LONG(ZEND_DIM_IS);

	REGISTER_INSPECTOR_LONG(ZEND_RETURNS_FUNCTION);
	REGISTER_INSPECTOR_LONG(ZEND_RETURNS_VALUE);
	REGISTER_INSPECTOR_LONG(ZEND_RETURN_VAL);
	REGISTER_INSPECTOR_LONG(ZEND_RETURN_REF);

	REGISTER_INSPECTOR_LONG(ZEND_ARRAY_ELEMENT_REF);
	REGISTER_INSPECTOR_LONG(ZEND_ARRAY_NOT_PACKED);
	REGISTER_INSPECTOR_LONG(ZEND_ARRAY_SIZE_SHIFT);
#undef REGISTER_INSPECTOR_LONG

	zend_execute_function = zend_execute_ex;
	zend_execute_ex = php_inspector_execute;

	return SUCCESS;
}
/* }}} */

/* {{{ */
PHP_MSHUTDOWN_FUNCTION(inspector)
{
	PHP_MSHUTDOWN(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MSHUTDOWN(inspector_strings)(INIT_FUNC_ARGS_PASSTHRU);

	zend_execute_ex = zend_execute_function;

	return SUCCESS;
} /* }}} */

static void php_inspector_table_free(zval *zv) {
	zend_hash_destroy(Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
}

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(inspector)
{
#if defined(COMPILE_DL_INSPECTOR) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	PHP_RINIT(inspector_map)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_RINIT(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_RINIT(inspector_file)(INIT_FUNC_ARGS_PASSTHRU);
	{
		zend_hash_init(&PIG(pending).file, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(pending).class, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(pending).function, 8, NULL, php_inspector_table_free, 0);

		zend_hash_init(&PIG(registered).file, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(registered).class, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(registered).function, 8, NULL, php_inspector_table_free, 0);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ */
PHP_RSHUTDOWN_FUNCTION(inspector)
{
	PHP_RSHUTDOWN(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_RSHUTDOWN(inspector_file)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_RSHUTDOWN(inspector_map)(INIT_FUNC_ARGS_PASSTHRU);
	{
		zend_hash_destroy(&PIG(pending).file);
		zend_hash_destroy(&PIG(pending).class);
		zend_hash_destroy(&PIG(pending).function);

		zend_hash_destroy(&PIG(registered).file);
		zend_hash_destroy(&PIG(registered).class);
		zend_hash_destroy(&PIG(registered).function);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(inspector)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "inspector support", "enabled");
	php_info_print_table_end();
}
/* }}} */

static const zend_module_dep inspector_deps[] = {
	ZEND_MOD_REQUIRED("Reflection")
        ZEND_MOD_END
};

ZEND_FUNCTION(Inspector_addressof)
{
	zval *variable;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z", &variable) != SUCCESS) {
		return;
	}

	switch (Z_TYPE_P(variable)) {
		case IS_OBJECT:
			RETURN_LONG((zend_long) Z_OBJ_P(variable));
		case IS_ARRAY:
			RETURN_LONG((zend_long) Z_ARR_P(variable));
		case IS_STRING:
			RETURN_LONG((zend_long) Z_STR_P(variable));
		case IS_RESOURCE:
			RETURN_LONG((zend_long) Z_RES_P(variable));
		
		default:
			RETURN_LONG((zend_long) variable);
	}
}

ZEND_BEGIN_ARG_INFO_EX(php_inspector_addressof_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, variable)
ZEND_END_ARG_INFO()

zend_function_entry php_inspector_functions[] = {
	ZEND_NS_FENTRY("Inspector", addressof, zif_Inspector_addressof, php_inspector_addressof_arginfo, 0)
	PHP_FE_END
};

/* {{{ inspector_module_entry
 */
zend_module_entry inspector_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	inspector_deps,
	"inspector",
	php_inspector_functions,
	PHP_MINIT(inspector),
	PHP_MSHUTDOWN(inspector),
	PHP_RINIT(inspector),
	PHP_RSHUTDOWN(inspector),
	PHP_MINFO(inspector),
	PHP_INSPECTOR_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_INSPECTOR
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(inspector)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
