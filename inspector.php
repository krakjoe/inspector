<?php
use Inspector\Inspector;
use Inspector\Node;
use Inspector\Operand;

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

function printOperand(Operand $op) {
	if ($op->isConstant()) {
		printConstant($op->getConstantValue());
	} else if ($op->isCompiledVariable()) {
		printf("\$%s\t", $op->getVariableName());
	} else if ($op->isTemporaryVariable()) {
		printf("T%d\t", $op->getVariableNumber());
	} else if($op->isVariable()) {
		printf("V%d\t", $op->getVariableNumber());
	} else printf("-\t");
}

function printInspector(Inspector $inspector) {
	printf("OPCODE\t\tOP1\tOP2\tRESULT\n");
	foreach ($inspector as $opline) {
		printf("%s\t", $opline->getType());
		printOperand($opline->getOperand(NODE::OP1));
		printOperand($opline->getOperand(NODE::OP2));
		printOperand($opline->getOperand(NODE::RESULT));
		printf("\n");
	}
}

printInspector(new Inspector(function($a, $b){
	return $a + $b;
}));
?>
