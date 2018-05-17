--TEST--
InspectorOpline getFunction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;

$inspector = 
	new InspectorFunction(function($a, $b) {});

$opline = 
	$inspector->getOpline();

if ($opline->getFunction() instanceof InspectorFunction) {
	echo "OK";
}
?>
--EXPECT--
OK

