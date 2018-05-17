--TEST--
InspectorFunction::getOpline out of bounds
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = new InspectorFunction(function(){});

try {
	$inspector->getOpline(-1);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
