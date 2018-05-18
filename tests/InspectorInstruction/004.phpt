--TEST--
InspectorInstruction getOpcode/getOpcodeName
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = 
	new InspectorFunction(function($a) {});

$opline = $inspector->getInstruction();

var_dump($opline->getOpcodeName(), 
	 $opline->getOpcode());
?>
--EXPECTF--
string(%d) "ZEND_%s"
int(%d)
