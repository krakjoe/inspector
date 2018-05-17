--TEST--
InspectorFunction::getOpline invalid parameters
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = new InspectorFunction(function(){});

try {
	$inspector->getOpline("string");
} catch (TypeError $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
