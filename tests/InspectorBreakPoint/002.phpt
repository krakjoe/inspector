--TEST--
InspectorBreakPoint enable/disable
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function foo($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction("foo");

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {}
};

var_dump($break->disable());
var_dump($break->disable());
var_dump($break->enable());
var_dump($break->enable());
var_dump($break->isEnabled());
?>
--EXPECT--
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
