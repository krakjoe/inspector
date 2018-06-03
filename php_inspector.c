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

	return zend_hash_update_ptr(rt, (zend_string*) key, table);
}

HashTable* php_inspector_table(php_inspector_root_t root, php_inspector_table_t type, zend_string *key, zend_bool create) {
	HashTable *rt = php_inspector_table_select(root, type);
	HashTable *table = 
		(HashTable*) zend_hash_find_ptr(rt, key);

	if (!table && create) {
		return php_inspector_table_create(root, type, key);
	}

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
	
	zend_hash_del(rt, key);
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
		if (map->run_time_cache)
			EX(run_time_cache) = map->run_time_cache;
#endif

#if ZEND_EX_USE_LITERALS
		EX(literals) = map->literals;
#endif
	}

	zend_execute_function(execute_data);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(inspector)
{
	ZEND_INIT_MODULE_GLOBALS(inspector, php_inspector_globals_init, NULL);

	PHP_MINIT(inspector_instruction_interface)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_strings)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_class)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_method)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_function)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_file)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_instruction)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_operand)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_frame)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_map)(INIT_FUNC_ARGS_PASSTHRU);

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

/* {{{ inspector_module_entry
 */
zend_module_entry inspector_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	inspector_deps,
	"inspector",
	NULL,
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
