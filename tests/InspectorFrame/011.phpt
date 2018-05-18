--TEST--
InspectorFrame getCall
--FILE--
<?php
use Inspector\InspectorInstruction;
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$func = function(){
	sprintf("nothing");
};

$in = new InspectorFunction($func);

$opline = $in->findFirstInstruction(InspectorInstruction::ZEND_DO_ICALL);

$break = new class ($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		if ($frame->getCall() instanceof InspectorFrame) {
			echo "OK";
		}
	}
};

$func();
?>
--EXPECT--
OK

