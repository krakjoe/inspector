--TEST--
InspectorFunction::findFirstInstruction non user code
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction("var_dump");

try {
	$inspector->findFirstInstruction(1);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
