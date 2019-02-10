--TEST--
InspectorFunction extends \ReflectionFunction
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = 
	new InspectorFunction("var_dump");

if ($inspector->getName() == "var_dump") {
	echo "OK";
}
?>
--EXPECT--
OK
