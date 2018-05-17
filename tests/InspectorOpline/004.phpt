--TEST--
InspectorOpline getOpcode/getOpcodeName
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;

$inspector = 
	new InspectorFunction(function($a) {});

$opline = $inspector->getOpline();

var_dump($opline->getOpcodeName(), 
	 $opline->getOpcode());
?>
--EXPECTF--
string(%d) "ZEND_%s"
int(%d)
