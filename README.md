Inspector
========
*Inspector allows you to programmatically inspect (and debug) PHP code*

[![Build Status](https://travis-ci.org/krakjoe/inspector.svg?branch=master)](https://travis-ci.org/krakjoe/inspector)

There are many tools that dump or print opcodes, but none of these tools allow programmatic inspection or debugging.

If you don't know what any of this is for, move on ... this is not for you ;)

API
===

The following API is provided:

```php
namespace Inspector
{
	abstract class Scope implements Traversable, Countable {
		public function getName() : ?string;
		public function getStatics() : array;
		public function getConstants() : array;
		public function getVariables() : array;
		public function getOpline(int num = 0) : Opline;
        	public function getLineStart() : int;
        	public function getLineEnd() : int;
		public function count() : int;

		public function getEntry() : ?Entry;
	}
	
	final class Func extends Scope {
		public function __construct(string function);
	}

	final class Method extends Scope {
		public function __construct(string class, string method);
	}

	final class Closure extends Scope {
		public function __construct(Closure closure);
	}

	final class File extends Scope {
		public function __construct(string filename [, bool clobber = false]);
		public function getEntries() : ?array;
		public function getGlobals() : ?array;
		public function getGlobal(string name) : ?Global;
		public function getEntry(string name) : ?Entry;
	}

	final class Entry implements Traversable, Countable {
		public function __construct(string class);
		public function getMethod(string name) : Method;
		public function getMethods() : array;
		public function count();
	}

	final class Opline {
		const OP1;
		const OP2;
		const RESULT;

		public function getType() : string;
		public function getOperand(int which) : Operand;
		public function getExtendedValue() : mixed;
		public function getLine() : int;
		public function getScope() : Scope;
		public function getNext() : ?Opline;
		public function getPrevious() : ?Opline;
		public function getBreakPoint() : BreakPoint;
	}

	final class Operand {
		public function isUnused() : bool;
		public function isExtendedTypeUnused() : bool;
		public function isCompiledVariable() : bool;
		public function isTemporaryVariable() : bool;
		public function isVariable() : bool;
		public function isConstant() : bool;
		public function isJumpTarget() : bool;
		public function getWhich() : int;
		public function getValue() : mixed;
		public function getName() : string;
		public function getNumber() : int;
		public function getOpline() : Opline;
	}

	abstract class BreakPoint {
		public function __construct(Opline $opline);
	
		public function enable() : bool;
		public function disable() : bool;
		public function isEnabled() : bool;

		public function getOpcode() : ?string;
		public function getOpline() : Opline;

		abstract public function hit(Frame $frame);
	}

	final class Frame {
		public function getScope() : Scope;
		public function getOpline() : Opline;
		public function getSymbols() : ?array;
		public function getPrevious() : ?Frame;
	}
}
```

Example
======
*Other people are better at this than I am ...*

Making stuff look nice is not my forte, all the same, here is some simple code using Inspector:

```php
use Inspector\Closure;
use Inspector\Scope;
use Inspector\Opline;
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
		printConstant($op->getValue());
	} else if ($op->isCompiledVariable()) {
		printf("\$%s\t", $op->getName());
	} else if ($op->isTemporaryVariable()) {
		printf("T%d\t", $op->getNumber());
	} else if($op->isVariable()) {
		printf("V%d\t", $op->getNumber());
	} else printf("-\t");
}

function printInspector(Scope $inspector) {
	printf("OPCODE\t\tOP1\tOP2\tRESULT\n");
	foreach ($inspector as $opline) {
		printf("%s\t", $opline->getType());
		printOperand($opline->getOperand(OPLINE::OP1));
		printOperand($opline->getOperand(OPLINE::OP2));
		printOperand($opline->getOperand(OPLINE::RESULT));
		printf("\n");
	}
}

printInspector(new Closure(function($a, $b){
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

 * moar Scope
 * moar Entry
 * tests (started)

**This extension is experimental, and requires PHP7**
