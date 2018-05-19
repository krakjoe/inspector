--TEST--
InspectorFrame getCurrent in global scope
--FILE--
<?php
use Inspector\InspectorInstruction;
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$test = new stdClass;

$frame = InspectorFrame::getCurrent();

$func = $frame->getFunction();

$stack = $frame->getStack();

if ($stack["test"] === $test) {
	echo "OK";
}
?>
--EXPECT--
OK

