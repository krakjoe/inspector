--TEST--
InspectorOpline getLine
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;

$inspector = 
	new InspectorFunction(function($a, $b) {});

$opline = 
	$inspector->getOpline();

var_dump($opline->getLine());
?>
--EXPECT--
int(6)

