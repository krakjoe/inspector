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
#ifndef HAVE_INSPECTOR_FILE_H
#define HAVE_INSPECTOR_FILE_H

zend_class_entry *php_inspector_file_ce;

/* {{{ */
PHP_METHOD(File, __construct)
{
	zend_string *filename = NULL;
	zend_file_handle fh;
	zend_op_array *ops = NULL;

	if (0) {
InvalidArgumentException:
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"%s expects (string filename)",
			ZSTR_VAL(EX(func)->common.function_name));
		return;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &filename) != SUCCESS) {
			return;
		} break;
		
		default:
			goto InvalidArgumentException;
	}

	if (zend_hash_exists(&EG(included_files), filename)) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot load already included file %s",
			ZSTR_VAL(filename));
		return;
	}

	if (php_stream_open_for_zend_ex(ZSTR_VAL(filename), &fh, USE_PATH|STREAM_OPEN_FOR_INCLUDE) != SUCCESS) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot open %s for parsing",
			ZSTR_VAL(filename));
		return;
	}

	if (!fh.opened_path) {
		fh.opened_path = zend_string_copy(filename);	
	}

	if (!zend_hash_add_empty_element(&EG(included_files), fh.opened_path)) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot add %s to included files table",
			ZSTR_VAL(filename));
		zend_file_handle_dtor(&fh);
		return;
	}

	ops = zend_compile_file(&fh, ZEND_REQUIRE);
	php_inspector_scope_construct(getThis(), (zend_function*) ops);
	zend_destroy_file_handle(&fh);
	destroy_op_array(ops);
	efree(ops);
}

static zend_function_entry php_inspector_file_methods[] = {
	PHP_ME(File, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(file) {
	zend_class_entry ce;
	
	INIT_NS_CLASS_ENTRY(ce, "Inspector", "File", php_inspector_file_methods);
	php_inspector_file_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_scope_ce);

	return SUCCESS;
} /* }}} */
#endif
