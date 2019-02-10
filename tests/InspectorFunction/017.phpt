--TEST--
InspectorFunction::findLastInstruction oob
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b){
	return $a + $b;
}

$inspector = new InspectorFunction("foo");

try {
	$inspector->findLastInstruction(InspectorInstruction::ZEND_ADD, 42);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
