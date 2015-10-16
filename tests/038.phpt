--TEST--
Inspector tests (pre/post inc/dec)
--SKIPIF--
<?php if (!extension_loaded("inspector")) print "skip"; ?>
--INI--
opcache.enable_cli=0
--FILE--
<?php 
require_once(sprintf("%s/inspector.inc", dirname(__FILE__)));

use Inspector\Inspector;

printInspector(new Inspector(function($a) {return ++$a;}));
?>
--EXPECTF--
RECV%w-%w-%w$a%w-
PRE_INC%w$a%w-%wV0%w-
RETURN%wV0%w-%w-%w-
RETURN%w-%w-%w-%w-
