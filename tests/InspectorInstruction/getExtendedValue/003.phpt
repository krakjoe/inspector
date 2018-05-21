--TEST--
InspectorInstruction getExtendedValue FETCH DEFAULT
--FILE--
<?php
use Inspector\InspectorMethod;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$inc = sprintf("%s/extended_003.inc", dirname(__FILE__));

$file = new class($inc) extends InspectorFile {
	public function onResolve() {
		$opline = $this->findFirstInstruction(InspectorInstruction::ZEND_FETCH_CLASS);

		if ($opline->getExtendedValue() == "default" &&
		    $opline->getOperand(InspectorOperand::OP2)->getValue() == "Super") {
			echo "OK";
		}
	}
};

include $inc;
?>
--EXPECT--
OK

