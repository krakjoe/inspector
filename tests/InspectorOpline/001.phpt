--TEST--
InspectorOpline getNext
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;

$inspector = 
	new InspectorFunction(function($a) {});

if ($inspector->getOpline()->getNext() instanceof InspectorOpline) {
	echo "OK";
}
?>
--EXPECT--
OK
