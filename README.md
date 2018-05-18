Inspector
========
*A disassembler and debug kit for PHP7*

[![Build Status](https://travis-ci.org/krakjoe/inspector.svg?branch=master)](https://travis-ci.org/krakjoe/inspector)
[![Coverage Status](https://coveralls.io/repos/github/krakjoe/inspector/badge.svg?branch=master)](https://coveralls.io/github/krakjoe/inspector?branch=master)

Inspector provides an API for disassembling and debugging PHP7 code, in userland.

API
===

The following API is provided:

```php
namespace Inspector
{
	class InspectorClass extends \ReflectionClass {
		public function getMethod(string name) : InspectorMethod;
		public function getMethods(int filter = 0) : array;
	}

	class InspectorMethod extends \ReflectionMethod implements InspectorInstructionInterface {
		public function getDeclaringClass() : InspectorClass;

		protected $instructionCache;
	}

	class InspectorFunction extends \ReflectionFunction implements InspectorInstructionInterface {
		protected $instructionCache;
	}

	interface InspectorInstructionInterface {
		public function getInstruction(int num = 0) : InspectorInstruction;
		public function getInstructionCount() : int;
		public function getEntryInstruction() : ?InspectorInstruction;
		public function findNextInstruction(int opcode, int offset = 0) : ?InspectorInstruction;
		public function findLastInstruction(int opcode, int offset = -1) : ?InspectorInstruction;
		public function flushInstructionCache();
	}

	final class InspectorInstruction {
		public function getOffset() : int;
		public function getOpcode() : int;
		public function getOpcodeName() : ?string;
		public function getOperand(int which) : InspectorOperand;
		public function getExtendedValue() : mixed;
		public function getLine() : int;
		public function getFunction() : InspectorFunction;
		public function getNext() : ?InspectorInstruction;
		public function getPrevious() : ?InspectorInstruction;
		public function getRelative(int offset) : ?InspectorInstruction;
		public function getBreakPoint() : InspectorBreakPoint;

		/* opcode constants are registered with prefixed ZEND_ */
	}

	final class InspectorOperand {
		const OP1;
		const OP2;
		const RESULT;

		public function isUnused() : bool;
		public function isExtendedTypeUnused() : bool;
		public function isCompiledVariable() : bool;
		public function isTemporaryVariable() : bool;
		public function isVariable() : bool;
		public function isConstant() : bool;
		public function isJumpTarget() : bool;
		public function getWhich() : int;
		public function getValue(?InspectorFrame $frame) : mixed;
		public function getName() : string;
		public function getNumber() : int;
		public function getInstruction() : InspectorInstruction;
	}

	abstract class InspectorBreakPoint {
		public function __construct(InspectorInstruction $opline);
	
		public function enable() : bool;
		public function disable() : bool;
		public function isEnabled() : bool;

		public function getOpcode() : int;
		public function getOpcodeName() : ?string;
		public function getInstruction() : InspectorInstruction;

		abstract public function hit(InspectorFrame $frame);
	}

	final class InspectorFrame {
		public function getFunction() : InspectorInstructionInterface;
		public function getInstruction() : InspectorInstruction;
		public function getSymbols() : ?array;
		public function getPrevious() : ?InspectorFrame;
		public function getCall() : ?InspectorFrame;
		public function getStack() : array;
		public function getParameters() : array;
		public function getVariable(int num) : mixed;
	}
}
```

Example
======
*Other people are better at this than I am ...*

Making stuff look nice is not my forte, all the same, here is some simple code using Inspector:

```php
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

printFunction(new InspectorFunction(function($a, $b){
	return $a + $b;
}));
```

Which will yield:

```
OPCODE          OP1     OP2     RESULT
ZEND_RECV       -       -       $a
ZEND_RECV       -       -       $b
ZEND_ADD        $a      $b      T1
ZEND_RETURN     T1      -       -
ZEND_RETURN     null    -       -
```

TODO
====

 * tests (started)
