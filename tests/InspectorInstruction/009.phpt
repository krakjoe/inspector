--TEST--
InspectorInstruction flushInstructionCache
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = 
	new InspectorFunction(function($a, $b) {
	$a + $b;
});

$opline = 
	$inspector->getInstruction();
$offset = 0;

do {
	if ($offset != $opline->getOffset()) {
		echo "FAIL {$offset}\n";
	}
	$offset++;
} while ($opline = $opline->getNext());

echo "OK\n";

var_dump($inspector);
$inspector->flushInstructionCache();
var_dump($inspector);
?>
--EXPECT--
OK
object(Inspector\InspectorFunction)#1 (2) {
  ["name"]=>
  string(9) "{closure}"
  ["instructionCache":protected]=>
  array(5) {
    [0]=>
    object(Inspector\InspectorInstruction)#3 (0) {
    }
    [1]=>
    object(Inspector\InspectorInstruction)#4 (0) {
    }
    [2]=>
    object(Inspector\InspectorInstruction)#5 (0) {
    }
    [3]=>
    object(Inspector\InspectorInstruction)#6 (0) {
    }
    [4]=>
    object(Inspector\InspectorInstruction)#7 (0) {
    }
  }
}
object(Inspector\InspectorFunction)#1 (2) {
  ["name"]=>
  string(9) "{closure}"
  ["instructionCache":protected]=>
  array(0) {
  }
}
