Inspector
========
*A disassembler and debug kit for PHP7*

[![Build Status](https://travis-ci.org/krakjoe/inspector.svg?branch=master)](https://travis-ci.org/krakjoe/inspector)

Inspector provides an API for disassembling and debugging PHP7 code, in userland.

API
===

The following API is provided:

```php
namespace Inspector
{
	class InspectorClass extends \ReflectionClass {
		public function getMethod() : InspectorMethod;
	}

	class InspectorMethod extends \ReflectionMethod {
		public function getOpline(int num 0) : InspectorOpline;

		public function getDeclaringClass() : InspectorClass;
	}

	class InspectorFunction extends \ReflectionFunction {
		public function getOpline(int num 0) : InspectorOpline;
	}

	final class InspectorOpline {
		const OP1;
		const OP2;
		const RESULT;

		public function getOpcode() : int;
		public function getOpcodeName() : ?string;
		public function getOperand(int which) : InspectorOperand;
		public function getExtendedValue() : mixed;
		public function getLine() : int;
		public function getFunction() : ReflectionFunctionAbstract;
		public function getNext() : ?InspectorOpline;
		public function getPrevious() : ?InspectorOpline;
		public function getBreakPoint() : InspectorBreakPoint;
	}

	final class InspectorOperand {
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
		public function getOpline() : InspectorOpline;
	}

	abstract class InspectorBreakPoint {
		public function __construct(InspectorOpline $opline);
	
		public function enable() : bool;
		public function disable() : bool;
		public function isEnabled() : bool;

		public function getOpcode() : int;
		public function getOpcodeName() : ?string;
		public function getOpline() : InspectorOpline;

		abstract public function hit(InspectorFrame $frame);
	}

	final class InspectorFrame {
		public function getFunction() : ReflectionFunctionAbstract;
		public function getOpline() : InspectorOpline;
		public function getSymbols() : ?array;
		public function getPrevious() : ?InspectorFrame;
		public function getCall() : ?InspectorFrame;
		public function getStack() : array;
		public function getParameters() : array;
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
use Inspector\InspectorOpline;
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
	$opline = $inspector->getOpline();

	do {
		printf("%s\t", $opline->getType());
		printOperand($opline->getOperand(InspectorOpline::OP1));
		printOperand($opline->getOperand(InspectorOpline::OP2));
		printOperand($opline->getOperand(InspectorOpline::RESULT));
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
