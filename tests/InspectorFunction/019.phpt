--TEST--
InspectorFunction::getInstructionCount non user code
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction("var_dump");

try {
	$inspector->getInstructionCount();
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
