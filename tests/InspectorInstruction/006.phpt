--TEST--
InspectorInstruction getLine
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = 
	new InspectorFunction(function($a, $b) {});

$opline = 
	$inspector->getInstruction();

var_dump($opline->getLine());
?>
--EXPECT--
int(6)

