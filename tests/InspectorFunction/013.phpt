--TEST--
InspectorFunction::findFirstInstruction negative oob
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b){
	return $a + $b;
}

$inspector = new InspectorFunction("foo");

try {
	$inspector->findFirstInstruction(InspectorInstruction::ZEND_ADD, -42);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
