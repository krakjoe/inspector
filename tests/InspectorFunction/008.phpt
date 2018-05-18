--TEST--
InspectorFunction::getEntryInstruction no entry
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = new InspectorFunction(function(){});

try {
	$inspector->getEntryInstruction();
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
