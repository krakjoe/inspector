--TEST--
InspectorFunction extends \ReflectionFunction
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = 
	new InspectorFunction("var_dump");

if ($inspector instanceof \ReflectionFunction) {
	echo "OK";
}
?>
--EXPECT--
OK
