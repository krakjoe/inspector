--TEST--
InspectorFunction::getOpline not user function
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = new InspectorFunction("var_dump");

try {
	$inspector->getOpline();
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
