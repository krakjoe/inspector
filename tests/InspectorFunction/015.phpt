--TEST--
InspectorFunction::findLastInstruction non user code
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction("var_dump");

try {
	$inspector->findLastInstruction(1);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
