<?php
use Inspector\Inspector;
use Inspector\Node;
use Inspector\Operand;

function printOpcode($name) {
	printf("%-20s\t", substr($name, 5));
}

function printConstant($mixed) {
	if ($mixed && strlen($mixed) > 8) {
		printf(
			"%s...\t\t", substr(str_replace(
			"\n",
			"\\n",
			$mixed
		), 0, 8));
	} else printf("%s\t\t", $mixed ?: "-");
}

function printOperand(Operand $op) {
	if ($op->isConstant()) {
		printConstant($op->getValue());
	} else if ($op->isCompiledVariable()) {
		printf("\$%s\t\t", $op->getName());
	} else if ($op->isTemporaryVariable()) {
		printf("T%d\t\t", $op->getNumber());
	} else if($op->isVariable()) {
		printf("V%d\t\t", $op->getNumber());
	} else if ($op->isJumpTarget()) {
		printf("J%d\t\t", $op->getNumber());
	} else printf("-\t\t");
}

function printExtendedValue($value) {
	printf("%s\n", $value ?: "-");
}

function printInspector(Inspector $inspector) {
	foreach ($inspector as $opline) {
		printOpcode($opline->getType());
		printOperand($opline->getOperand(NODE::OP1));
		printOperand($opline->getOperand(NODE::OP2));
		printOperand($opline->getOperand(NODE::RESULT));
		printExtendedValue($opline->getExtendedValue());
	}
}
