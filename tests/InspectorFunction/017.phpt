--TEST--
InspectorFunction::findLastInstruction oob
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction(function($a, $b){
	return $a + $b;
});

try {
	$inspector->findLastInstruction(InspectorInstruction::ZEND_ADD, 42);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
