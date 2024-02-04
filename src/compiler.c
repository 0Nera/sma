#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 64 * 1024
#define MAX_LINE_LENGTH 200
#define BOOT_FILE "disk.hdd"

typedef struct {
	char name[5];
	int num_params;
	int opcode;
} instruction;

typedef struct {
	char name[5];
	int opcode;
} reg;

instruction instructions[] = {
	{ "OUT", 1, 'P' }, { "HALT", 0, 'H' }, { "ENA", 2, 'O' },  { "INC", 1, 'I' }, { "DEC", 1, 'D' },
	{ "CLR", 1, 'N' }, { "JMP", 1, 'J' },  { "ADDN", 2, '+' }, { "ADD", 2, 'A' }, { "SUB", 2, 'S' },
	{ "MUL", 2, 'M' }, { "DIV", 2, 'L' },  { "CMP", 2, 'C' },  { "MOV", 2, 'R' },
};

reg registers[] = {
	{ "R0", '0' }, { "R1", '1' }, { "R2", '2' }, { "R3", '3' }, { "IP", 'P' }, { "CID", 'C' }, { "ERR", 'E' },
};

int line_num = -1;
char command[5];
char parameter1[5];
char parameter2[5];
FILE *output_file;

void trim_leading_spaces(char *str) {
	int i = 0;
	while (str[i] != '\0' && isspace(str[i])) { i++; }
	memmove(str, str + i, strlen(str) - i + 1);
}

void remove_comment(char *str) {
	int i = 0;
	while (str[i] != '\0' && str[i] != ';') { i++; }
	while (str[i] != '\0') {
		str[i] = '\0';
		i++;
	}
}

void compile_command(char *_command, char *parameter1, char *parameter2, char *memory) {
	for (int i = 0; i < sizeof(instructions) / sizeof(instruction); i++) {
		if (strcmp(_command, instructions[i].name) == 0) {
			char opcode = instructions[i].opcode;
			int n1 = 0;
			int n2 = 0;
			char *pend;
			reg *reg_num1 = NULL;
			reg *reg_num2 = NULL;

			if (instructions[i].num_params > 0) {
				n1 = strtol(parameter1, &pend, 10);
				for (int j = 0; j < sizeof(registers) / sizeof(reg); j++) {
					if (strcmp(parameter1, registers[j].name) == 0) {
						reg_num1 = &registers[j];
						break;
					}
				}
			}

			if (instructions[i].num_params > 1) {
				n2 = strtol(parameter2, &pend, 10);
				for (int j = 0; j < sizeof(registers) / sizeof(reg); j++) {
					if (strcmp(parameter2, registers[j].name) == 0) {
						reg_num2 = &registers[j];
						break;
					}
				}
			}

			switch (instructions[i].num_params) {
				case 0:
					printf("[%c]\n", opcode);
					fprintf(output_file, "%c\n", opcode);
					break;
				case 1:
					if (reg_num1 == NULL) {
						printf("[%c]<%hd>\n", opcode, n1);
						fprintf(output_file, "%c%hd\n", opcode, n1);
					} else {
						printf("[%c]<%s>\n", opcode, reg_num1->name);
						fprintf(output_file, "%c%c\n", opcode, reg_num1->opcode);
					}
					break;
				case 2:
					if (reg_num1 == NULL) {
						if (reg_num2 == NULL) {
							printf("[%c]<%hd><%hd>\n", opcode, n1, n2);
							fprintf(output_file, "%c%hd%hd\n", opcode, n1, n2);
						} else {
							printf("[%c]<%hd><%s>\n", opcode, n1, reg_num2->name);
							fprintf(output_file, "%c%hd%c\n", opcode, n1, reg_num2->opcode);
						}
					} else {
						if (reg_num2 == NULL) {
							printf("[%c]<%s><%hd>\n", opcode, reg_num1->name, n2);
							fprintf(output_file, "%c%c%hd\n", opcode, reg_num1->opcode, n2);
						} else {
							printf("[%c]<%s><%s>\n", opcode, reg_num1->name, reg_num2->name);
							fprintf(output_file, "%c%c%c\n", opcode, reg_num1->opcode, reg_num2->opcode);
						}
					}
					break;
			}
			return;
		}
	}

	printf("Error: Unknown command '%s' <%s> <%s>, line: %d\n", _command, parameter1, parameter2, line_num);
	exit(1);
}

void compile(const char *input_filename, const char *output_filename) {
	FILE *input_file = fopen(input_filename, "r");
	if (input_file == NULL) {
		printf("Error: Failed to open file '%s'\n", input_filename);
		exit(1);
	}

	char *memory = (char *)malloc(MEMORY_SIZE);
	char *memory_orig = memory;
	char line[MAX_LINE_LENGTH];

	memset(memory, '\n', MEMORY_SIZE);

	output_file = fopen(output_filename, "wb");

	if (output_file == NULL) {
		printf("Error: Failed to create file '%s'\n", output_filename);
		exit(1);
	}

	while (fgets(line, sizeof(line), input_file)) {
		line_num++;
		trim_leading_spaces(line);
		remove_comment(line);

		if (strlen(line) < 1) { continue; }

		sscanf(line, "%s %s %s", command, parameter1, parameter2);
		compile_command(command, parameter1, parameter2, (char *)memory);
	}

	fclose(input_file);

	fclose(output_file);
}

int main( ) {
	compile("prog.asm", BOOT_FILE);
	printf("Compilation successful. File '%s' generated.\n", BOOT_FILE);

	return 0;
}
