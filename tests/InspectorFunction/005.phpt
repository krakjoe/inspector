--TEST--
InspectorFunction::getInstruction invalid parameters
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = new InspectorFunction(function(){});

try {
	$inspector->getInstruction("string");
} catch (TypeError $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
