--TEST--
InspectorFunction::getOpline returns InspectorOpline
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;

$inspector = 
	new InspectorFunction(function(){});

if ($inspector->getOpline() instanceof InspectorOpline) {
	echo "OK";
}
?>
--EXPECT--
OK
