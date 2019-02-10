--TEST--
InspectorInstruction flushInstructionCache
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b) {
	$a + $b;
}

$inspector = new InspectorFunction("foo");

$opline = 
	$inspector->getInstruction();
$offset = 0;

do {
	if ($offset != $opline->getOffset()) {
		echo "FAIL {$offset}\n";
	}
	$offset++;
} while ($opline = $opline->getNext());

if ($inspector->flushInstructionCache() > 1)
	echo "OK\n";
?>
--EXPECT--
OK
