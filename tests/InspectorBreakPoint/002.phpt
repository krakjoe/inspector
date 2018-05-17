--TEST--
InspectorBreakPoint enable/disable
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {}
};

if ($break->disable() == true &&
    $break->disable() == false &&
    $break->enable() == true &&
    $break->enable() == false &&
    $break->isEnabled() == true) {
	echo "OK";
}
?>
--EXPECT--
OK
