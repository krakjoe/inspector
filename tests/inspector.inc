<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

function printConstant($mixed) {
	if (strlen($mixed) > 8) {
		printf(
			"%s...\t", substr(str_replace(
			"\n",
			"\\n",
			$mixed
		), 0, 8));
	} else printf("%s\t", $mixed ?: "null");
}

function printOperand(InspectorOperand $op) {
	if ($op->isConstant()) {
		printConstant($op->getValue());
	} else if ($op->isCompiledVariable()) {
		printf("\$%s\t", $op->getName());
	} else if ($op->isTemporaryVariable()) {
		printf("T%d\t", $op->getNumber());
	} else if($op->isVariable()) {
		printf("V%d\t", $op->getNumber());
	} else printf("-\t");
}

function printFunction(InspectorFunction $inspector) {
	printf("OPCODE\t\tOP1\tOP2\tRESULT\n");
	$opline = $inspector->getInstruction();

	do {
		printf("%s\t", $opline->getOpcodeName());
		printOperand($opline->getOperand(InspectorOperand::OP1));
		printOperand($opline->getOperand(InspectorOperand::OP2));
		printOperand($opline->getOperand(InspectorOperand::RESULT));
		printf("\n");
	} while ($opline = $opline->getNext());
}
