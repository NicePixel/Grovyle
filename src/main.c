#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <getopt.h>
#include "file.h"

#define INSTRUCTION_OP_ZERO 'Z'
#define INSTRUCTION_OP_SUCCESSOR 'S'
#define INSTRUCTION_OP_JUMP 'J'
#define INSTRUCTION_OP_TRANSFER 'T'
#define INSTRUCTION_OP_HALT 'H' /* Only returned by findinstruction. */
struct Instruction
{
	unsigned number, arg[3];
	char op;
};

struct Program
{
	struct Instruction* instructions;
	size_t instructioncount;
	size_t codesize;
	char* code;
};

/* Options */
static unsigned int outputregisterindex = 0;
static int verboseoutput = 0;
static int printonlyfinalvalue = 0;
static int manualstep = 0;

#define REGISTER_AMOUNT (1u << 8)
static uint64_t registers[REGISTER_AMOUNT];
static unsigned int relevantregisters[REGISTER_AMOUNT], relevantregisterscount;
static struct Program program;

static void
usage(char* program)
{
	printf("Usage: %s [OPTIONS] FILE INITIAL_STATE\n", program);
	puts("Options:");
	puts("-v\t Verbose output messages.");
	puts("-r\t Only output the final value of register 1 to stdout.");
	puts("-s\t Manually call instructions one by one.");
	puts("-o n\t Set the simulation's output register index to n. (default 1)");
	puts("-h\t Help messages.");
	puts("INITIAL_STATE is one string argument in format \"R1=8 R42=9 (...) Rn=x\".\n\
This sets up the initial state of the machine(registers).");
}

static void
usage_manualstep(char* command)
{
	printf("\"%s\" is not a valid command. Valid commands include:\n", command);
	puts("q \t Immediately exit the program.");
	puts("p \t Print used registers.");
	puts("s \t One step. Execute the next instruction.");
	puts("c \t Continue. Start executing the program without manual intervention.");
}

static int
prepareprogram(const char* filepath)
{
	size_t i;
	if (fileread(filepath, &program.code, &program.codesize))
	{
		return 1;
	}
	program.instructioncount = 0;
	for (i = 0; i < program.codesize; i++)
	{
		if (program.code[i] == '\n')
		{
			program.instructioncount++;
		}
	}
	return 0;
}

static int
preparemachine(const char* args)
{
	size_t argoffset = 0;
	while (args[argoffset] != '\0')
	{
		uint64_t value;
		size_t registernumber;
		if (sscanf(args+argoffset, " R%ld=%"PRId64"", &registernumber, &value) != 2)
		{
			fprintf(stderr, "Malformed argument for machine's initial state!\n");
			return 1;
		}
		registers[registernumber-1] = value;
		if (verboseoutput)
		{
			printf("LET R[%ld]=%"PRId64"\n", registernumber, value);
		}
		/* Move to the next argument (if it exists) */
		while (args[argoffset] != ' ' && args[argoffset] != '\t' && args[argoffset] != '\n' && args[argoffset] != '\0')
		{
			argoffset++;
		}
		if (args[argoffset] != '\0')
		{
			argoffset++;
		}
	}
	return 0;
}

static int
parseargs(int argc, char** argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "vrhso:")) != EOF)
	{
		switch(opt)
		{
		case 'v':
			verboseoutput = 1;
			break;
		case 'r':
			verboseoutput = 0; /* if -r precedes -v, this will do nothing. */
			printonlyfinalvalue = 1;
			break;
		case 's':
			manualstep = 1;
			break;
		case 'o':
			if (sscanf(optarg, " %u", &outputregisterindex) != 1)
			{
				fprintf(stderr, "Erroneous output index. Given: \"%s\".\n", optarg);
				usage(argv[0]);
				return 1;
			}
			outputregisterindex--;
			break;
		case 'h':
		default:
			usage(argv[0]);
			return 1;
		}
	}
	/* File path argument */
	if (optind >= argc)
	{
		fputs("File path was not given.\n", stderr);
		usage(argv[0]);
		return 1;
	}
	if (prepareprogram(argv[optind]))
	{
		return 1;
	}
	/* Initial state argument (additional args) */
	if (argc > optind+1)
	{
		if (preparemachine(argv[optind+1]))
		{
			return 1;
		}
	}
	return 0;
}

static void
addrelevantregister(unsigned int registerindex)
{
	unsigned int i;
	for (i = 0; i < relevantregisterscount; i++)
	{
		if (relevantregisters[i] == registerindex)
		{
			return;
		}
	}
	relevantregisters[i] = registerindex;
	relevantregisterscount++;
}

static inline void
printrelevantregisters(void)
{
	unsigned int i;
	for (i = 0; i < relevantregisterscount; i++)
	{
		printf("R%u = %lu\n", i+1, registers[relevantregisters[i]]);
	}
}

static int
readprogram(int verboseoutput)
{
	size_t program_codeoffset, readinstructionlines, instructionindex;
	program.instructions = malloc(program.instructioncount*sizeof(struct Instruction));

	relevantregisterscount = 0;
	program_codeoffset = readinstructionlines = instructionindex = 0;
	for (;;)
	{
		const size_t instructionstart = program_codeoffset;
		size_t instructionstringlength;
		int parsedamount;
		char* instructioncode;

		/*
		 * Let `instructioncode` be the next read instruction.
		 * Assuming every instruction has its own separate line.
		 */
		while (program.code[program_codeoffset] != '\n')
		{
			program_codeoffset++;
		}
		instructionstringlength = program_codeoffset-instructionstart;
		instructioncode         = malloc((instructionstringlength+1)*sizeof(char));
		memcpy(instructioncode, program.code+instructionstart, instructionstringlength);
		instructioncode[instructionstringlength] = '\0';

		parsedamount = sscanf(instructioncode, "%d. %c ( %d , %d , %d )",
			&program.instructions[instructionindex].number,
			&program.instructions[instructionindex].op,
			&(program.instructions[instructionindex].arg[0]),
			&(program.instructions[instructionindex].arg[1]),
			&(program.instructions[instructionindex].arg[2]));
		if (parsedamount == 2 && program.instructions[instructionindex].op == INSTRUCTION_OP_ZERO)
		{
			fprintf(stderr, "Line number %ld: operator '%c' requires one argument.\n", readinstructionlines, INSTRUCTION_OP_ZERO);
			return 1;
		}
		else if (parsedamount == 2 && program.instructions[instructionindex].op == INSTRUCTION_OP_SUCCESSOR)
		{
			fprintf(stderr, "Line number %ld: operator '%c' requires one argument.\n", readinstructionlines, INSTRUCTION_OP_SUCCESSOR);
			return 2;
		}
		else if (parsedamount < 4 && program.instructions[instructionindex].op == INSTRUCTION_OP_TRANSFER)
		{
			fprintf(stderr, "Line number %ld: operator '%c' requires two arguments.\n", readinstructionlines, INSTRUCTION_OP_TRANSFER);
			return 3;
		}
		else if (parsedamount < 5 && program.instructions[instructionindex].op == INSTRUCTION_OP_JUMP)
		{
			fprintf(stderr, "Line number %ld: operator '%c' requires three arguments.\n", readinstructionlines, INSTRUCTION_OP_JUMP);
			return 3;
		}
		else if (parsedamount <= 0)
		{
			fprintf(stderr, "Line number %ld: erroneous format.\n", readinstructionlines);
			return 4;
		}
		switch (program.instructions[instructionindex].op)
		{
			default:
			case INSTRUCTION_OP_JUMP:
			case INSTRUCTION_OP_TRANSFER:
				addrelevantregister(program.instructions[instructionindex].arg[1]-1);
			case INSTRUCTION_OP_SUCCESSOR:
			case INSTRUCTION_OP_ZERO:
				addrelevantregister(program.instructions[instructionindex].arg[0]-1);
				break;
			case INSTRUCTION_OP_HALT:
				puts("[Exited the previous instruction.])");
		}
		if (verboseoutput)
		{
			printf("-> %4d. Operator %c. Parameters: %d %d %d;\n",
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
		printf("Read program has %lu instructions. Output is looked at register R%d.\n", program.instructioncount, outputregisterindex+1);
	}
	return 0;
}

/*
 * Return the instruction with the given number.
 * If no such instruction exists, return an instruction with `op` of 'H'.
 */
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
	halt_ins.op = INSTRUCTION_OP_HALT;
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
		if (manualstep)
		{
			char in[256];
			int loopinput;
			printf("-> %c(", ins.op);
			switch (ins.op)
			{
				default:
				case INSTRUCTION_OP_JUMP:      printf("%d, %d, %d)\n", ins.arg[0], ins.arg[1], ins.arg[2]); break;
				case INSTRUCTION_OP_TRANSFER:  printf("%d, %d)\n",     ins.arg[0], ins.arg[1]);             break;
				case INSTRUCTION_OP_SUCCESSOR: printf("%d)\n",         ins.arg[0]);                         break;
				case INSTRUCTION_OP_ZERO:      printf("%d)\n",         ins.arg[0]);                         break;
				case INSTRUCTION_OP_HALT:      puts("[Program halted.])");                                  break;
			}
			do
			{
				int inlength;
				printf(": ");
				fgets(in, 256, stdin);
				inlength = strlen(in);
				in[inlength-1] = '\0';
				inlength--;
				if (inlength == 0)
				{
					loopinput = 1;
					continue;
				}
				loopinput = 0;
				switch(in[0])
				{
					case 'q':
						running = 0;
						break;
					default:
						usage_manualstep(in);
						loopinput = 1;
					case 'c':
						manualstep = 0;
					case 's':
						break;
					case 'p':
						printrelevantregisters();
						loopinput = 1;
						break;
				}
			} while (loopinput);
			if (!running)
			{
				break;
			}
		}
		switch(ins.op)
		{
			case INSTRUCTION_OP_JUMP:
				if (registers[ins.arg[0]-1] == registers[ins.arg[1]-1])
				{
					ip = ins.arg[2];
				}
				else
				{
					ip++;
				}
				break;
			case INSTRUCTION_OP_SUCCESSOR:
				registers[ins.arg[0]-1]++;
				ip++;
				break;
			case INSTRUCTION_OP_ZERO:
				registers[ins.arg[0]-1] = 0;
				ip++;
				break;
			case INSTRUCTION_OP_TRANSFER:
				registers[ins.arg[1]-1] = registers[ins.arg[0]-1];
				ip++;
				break;
			case INSTRUCTION_OP_HALT:
				/* In manual mode, the user has to quit explicitly. */
				if (!manualstep)
				{
					running = 0;
				}
				break;
		}
	}
	if (printonlyfinalvalue)
	{
		printf("%"PRId64"\n", registers[outputregisterindex]);
	}
	else
	{
		printf("Register [%u] = %"PRId64"\n", outputregisterindex+1, registers[outputregisterindex]);
	}
	return registers[outputregisterindex];
}

int
main(int argc, char** argv)
{
	if (parseargs(argc, argv))
	{
		return 1;
	}
	if (readprogram(verboseoutput))
	{
		return 2;
	}
	executeprogram();
	return 0;
}
