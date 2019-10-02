#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

struct Instruction
{
	unsigned int number, arg[3];
	char op;
};
struct Program
{
	struct Instruction* instructions;
	size_t instructioncount;
	char* code;
};

#define REGISTER_AMOUNT (1u << 8)
static uint64_t registers[REGISTER_AMOUNT];
static struct Program program;

static int
readprogram(int verboseoutput)
{
	size_t program_codeoffset, readinstructionlines, instructionindex;
	program.instructioncount = 6;
	program.code =
"1. Z(1)\n"
"2. S(1)\n"
"3. S(1)\n"
"4. J(2, 2, 5)\n"
"5. S(2)\n"
"6. J(1, 1, 100)\n";
	program.instructions = malloc(program.instructioncount*sizeof(struct Instruction));

	program_codeoffset = readinstructionlines = instructionindex = 0;
	for (;;)
	{
		const size_t instructionstart = program_codeoffset;
		size_t instructionstringlength;
		int parsedamount;
		char* instructioncode;

		while (program.code[program_codeoffset] != '\n')
		{
			program_codeoffset++;
		}

		instructionstringlength = program_codeoffset-instructionstart;
		instructioncode         = malloc((instructionstringlength+1)*sizeof(char));
		memcpy(instructioncode, program.code+instructionstart, instructionstringlength);
		instructioncode[instructionstringlength] = '\0';

		parsedamount = sscanf(instructioncode, "%d. %c(%d, %d, %d)",
			&program.instructions[instructionindex].number,
			&program.instructions[instructionindex].op,
			&(program.instructions[instructionindex].arg[0]),
			&(program.instructions[instructionindex].arg[1]),
			&(program.instructions[instructionindex].arg[2]));
		if (parsedamount == 2 && program.instructions[instructionindex].op == 'Z')
		{
			printf("Line number %ld: operator 'Z' requires one argument.\n", readinstructionlines);
			return 1;
		}
		else if (parsedamount == 2 && program.instructions[instructionindex].op == 'S')
		{
			printf("Line number %ld: operator 'S' requires one argument.\n", readinstructionlines);
			return 2;
		}
		else if (parsedamount < 4 && program.instructions[instructionindex].op == 'T')
		{
			printf("Line number %ld: operator 'T' requires two arguments.\n", readinstructionlines);
			return 3;
		}
		else if (parsedamount < 5 && program.instructions[instructionindex].op == 'J')
		{
			printf("Line number %ld: operator 'J' requires three arguments.\n", readinstructionlines);
			return 3;
		}
		if (verboseoutput)
		{
			printf("-> %d. Operator %c. Parameters: %d %d %d;\n",
				program.instructions[instructionindex].number,
				program.instructions[instructionindex].op,
				program.instructions[instructionindex].arg[0],
				program.instructions[instructionindex].arg[1],
				program.instructions[instructionindex].arg[2]);
		}

		instructionindex++;
		program_codeoffset++; /* Skip the newline character. */
		readinstructionlines++;
		if (readinstructionlines == program.instructioncount)
		{
			break;
		}
	}
	if (verboseoutput)
	{
		printf("Read program has %lu instructions.\n", program.instructioncount);
	}
	return 0;
}

static struct Instruction
findinstruction(size_t number)
{
	struct Instruction halt_ins;
	size_t i;
	for (i = 0; i < program.instructioncount; i++)
	{
		const struct Instruction ins = program.instructions[i];
		if (ins.number == number)
		{
			return ins;
		}
	}
	halt_ins.op = 'H';
	return halt_ins;
}

static int
executeprogram(void)
{
	size_t ip; /* Instruction pointer. */
	int running;
	ip = 1;
	running = 1;
	while (running)
	{
		struct Instruction ins = findinstruction(ip);
		switch(ins.op)
		{
			case 'J':
				if (registers[ins.arg[0]-1] == registers[ins.arg[1]-1])
				{
					ip = ins.arg[2];
				}
				else
				{
					ip++;
				}
				break;
			case 'S':
				registers[ins.arg[0]-1]++;
				ip++;
				break;
			case 'Z':
				registers[ins.arg[0]-1] = 0;
				ip++;
				break;
			case 'T':
				registers[ins.arg[1]-1] = registers[ins.arg[0]-1];
				ip++;
				break;
			case 'H':
				running = 0;
				break;
		}
	}
	printf("Register [1] = %"PRId64"\n", registers[0]);
	return registers[0];
}

int
main(int argc, char** argv)
{
	int verboseoutput = 1;
	if (readprogram(verboseoutput))
	{
		return 1;
	}
	executeprogram();
	return 0;
}
