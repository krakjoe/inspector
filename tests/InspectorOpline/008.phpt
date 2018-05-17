--TEST--
InspectorOpline getOffset
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;

$inspector = 
	new InspectorFunction(function($a, $b) {
	$a + $b;
});

$opline = 
	$inspector->getOpline();
$offset = 0;

do {
	if ($offset != $opline->getOffset()) {
		echo "FAIL {$offset}\n";
	}
	$offset++;
} while ($opline = $opline->getNext());

echo "OK";
?>
--EXPECT--
OK

