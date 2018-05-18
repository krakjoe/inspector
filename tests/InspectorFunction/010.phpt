--TEST--
InspectorFunction::getEntryInstruction non user code
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction("var_dump");

try {
	$inspector->getEntryInstruction();
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
