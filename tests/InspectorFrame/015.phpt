--TEST--
InspectorFrame getCurrent in object scope
--FILE--
<?php
use Inspector\InspectorInstruction;
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

class Test {
	public function method(Closure $dispatch) {
		$dispatch(InspectorFrame::getCurrent());
	}
}

$test = new Test();

$test->method(function(InspectorFrame $frame) use($test) {
	if ($frame->getThis() == $test) {
		echo "OK";
	}
});
?>
--EXPECT--
OK

