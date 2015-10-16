--TEST--
Inspector tests (basics)
--SKIPIF--
<?php if (!extension_loaded("inspector")) print "skip"; ?>
--INI--
opcache.enable_cli=0
--FILE--
<?php 
require_once(sprintf("%s/inspector.inc", dirname(__FILE__)));

use Inspector\Closure;

printInspector(new Closure(function($a, $b) {return $a != $b;}));
?>
--EXPECTF--
RECV%w-%w-%w$a%w-
RECV%w-%w-%w$b%w-
IS_NOT_EQUAL%w$a%w$b%wT1%w-
RETURN%wT1%w-%w-%w-
RETURN%w-%w-%w-%w-
