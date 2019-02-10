--TEST--
InspectorFrame getCall
--FILE--
<?php
use Inspector\InspectorInstruction;
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function foo(){
	sprintf("nothing");
};

$in = new InspectorFunction("foo");

$opline = $in->findFirstInstruction(InspectorInstruction::ZEND_DO_ICALL);

$break = new class ($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		if ($frame->getCall() instanceof InspectorFrame) {
			echo "OK";
		}
	}
};

foo();
?>
--EXPECT--
OK

