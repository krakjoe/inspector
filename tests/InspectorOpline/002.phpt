--TEST--
InspectorOpline getPrevious
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;

$inspector = 
	new InspectorFunction(function($a) {});

if ($inspector->getOpline()->getNext()->getPrevious() instanceof InspectorOpline) {
	echo "OK";
}
?>
--EXPECT--
OK