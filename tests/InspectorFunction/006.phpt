--TEST--
InspectorFunction::getInstruction not user function
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = new InspectorFunction("var_dump");

try {
	$inspector->getInstruction();
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
