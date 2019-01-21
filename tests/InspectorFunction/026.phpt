--TEST--
InspectorFunction onTrace test
--INI--
opcache.enabled=0
--FILE--
<?php
use Inspector\InspectorFunction;

/* start source */
function add($one, $two) {
	try {
		if ($one + $two > 3) {
			return 4;
		}
	} catch (Exception $ex) {
		throw $ex;
	} finally {
		if ($one + $two > 42) {
			throw new Exception();
		}
	}
	return 5;
}
/* end source */

/* set tracer */
$function = new class("add") extends InspectorFunction {
	public function onResolve() {
		$this->reset();
	}

	public function onTrace(Inspector\InspectorFrame $frame) {
		$offset = $frame->getInstruction()->getOffset();

		if (!isset($this->trace[$offset])) {
			return;
		}

		$this->trace[$offset]++;
	}

	public function printTrace() {
		$instruction = $this->getEntryInstruction();
		printf("Line\tOffset\t%-20s Hit\n",
				"Opcode");
		printf("----------------------------------------\n");
		do {
			printf("L%d\t#%d\t%-20s %d\n",
				$instruction->getLine(),
				$instruction->getOffset(),
				$instruction->getOpcodeName(),
				$this->trace[$instruction->getOffset()]);
		} while ($instruction = $instruction->getNext());
	}

	public function reset() {
		$instruction = $this->getEntryInstruction();

		do {
			$this->trace[$instruction->getOffset()] = 0;
		} while ($instruction = $instruction->getNext());
	}

	private $trace;
};

/* start test */
add(1,2);
/* end test */

$function->printTrace();

$function->reset();

/* start next test */
add(3,4);
/* end next test */

$function->printTrace();

$function->reset();

/* start next test */
try {
	add(42,42);
} catch (Exception $ex) {}
/* end next test */

$function->printTrace();

?>
--EXPECT--
Line	Offset	Opcode               Hit
----------------------------------------
L7	#2	ADD                  1
L7	#3	IS_SMALLER           1
L7	#4	JMPZ                 0
L8	#5	FAST_CALL            0
L8	#6	RETURN               0
L8	#7	JMP                  1
L10	#8	CATCH                0
L11	#9	THROW                0
L12	#10	FAST_CALL            1
L12	#11	JMP                  1
L13	#12	ADD                  1
L13	#13	IS_SMALLER           1
L13	#14	JMPZ                 0
L14	#15	NEW                  0
L14	#16	DO_FCALL             0
L14	#17	THROW                0
L14	#18	FAST_RET             1
L17	#19	RETURN               1
L18	#20	RETURN               0
Line	Offset	Opcode               Hit
----------------------------------------
L7	#2	ADD                  1
L7	#3	IS_SMALLER           1
L7	#4	JMPZ                 0
L8	#5	FAST_CALL            1
L8	#6	RETURN               1
L8	#7	JMP                  0
L10	#8	CATCH                0
L11	#9	THROW                0
L12	#10	FAST_CALL            0
L12	#11	JMP                  0
L13	#12	ADD                  1
L13	#13	IS_SMALLER           1
L13	#14	JMPZ                 0
L14	#15	NEW                  0
L14	#16	DO_FCALL             0
L14	#17	THROW                0
L14	#18	FAST_RET             1
L17	#19	RETURN               0
L18	#20	RETURN               0
Line	Offset	Opcode               Hit
----------------------------------------
L7	#2	ADD                  1
L7	#3	IS_SMALLER           1
L7	#4	JMPZ                 0
L8	#5	FAST_CALL            1
L8	#6	RETURN               0
L8	#7	JMP                  0
L10	#8	CATCH                0
L11	#9	THROW                0
L12	#10	FAST_CALL            0
L12	#11	JMP                  0
L13	#12	ADD                  1
L13	#13	IS_SMALLER           1
L13	#14	JMPZ                 0
L14	#15	NEW                  1
L14	#16	DO_FCALL             1
L14	#17	THROW                1
L14	#18	FAST_RET             0
L17	#19	RETURN               0
L18	#20	RETURN               0

