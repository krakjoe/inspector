--TEST--
InspectorFunction::findFirstInstruction negative
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction(function($a, $b){
	return $a + $b;
});

try {
	$inspector->findFirstInstruction(InspectorInstruction::ZEND_ADD, 42);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
