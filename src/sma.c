#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CPU_NAME "SMA0"
#define THREAD_MAX 8
#define BOOT_FILE "disk.hdd"
#define MEM_SIZE 64 * 1024

typedef int16_t reg_t;

struct context {
	reg_t usable[4];
	reg_t instruction_pointer;
	reg_t core_id;
	reg_t err_id;
	uint8_t state;
};

struct context avaible_cores[THREAD_MAX];
struct context *cores;
char *memory = NULL;
uint8_t running_core = 1;

reg_t *get_reg(char i) {
	switch (i) {
		case '0': return &(cores->usable[0]);
		case '1': return &(cores->usable[1]);
		case '2': return &(cores->usable[2]);
		case '3': return &(cores->usable[3]);
		case 'P': return &(cores->instruction_pointer);
		case 'C': return &(cores->core_id);
		case 'E': return &(cores->err_id);
		default: printf("Error! Invalid register\n"); exit(-1);
	}
}

void execute_instruction(char instruction) {
	cores->instruction_pointer++;

	switch (instruction) {
		case 'P': // Вывод значения из регистра на экран
		{
			char reg_num = memory[cores->instruction_pointer];
			reg_t *r = get_reg(reg_num);
			printf("Value: %u\n", (int)*r);
			cores->instruction_pointer++;
		} break;
		case 'H': // Остановка потока
		{
			cores->state = 2;
			running_core--;
		} break;
		case 'O': // Включение потока процессора
		{
			char core_num = memory[cores->instruction_pointer] - '0'; // Получаем номер ядра
			char reg_num = memory[cores->instruction_pointer + 1];
			reg_t *r = get_reg(reg_num);
			// printf("%u, %c = %u\n", core_num, reg_num, *r);
			avaible_cores[core_num].state = 1;
			avaible_cores[core_num].instruction_pointer = *r;
			cores->instruction_pointer += 2;
			running_core++;
		} break;
		case 'I': // Инкремент значения регистра
		{
			char reg_num = memory[cores->instruction_pointer];
			reg_t *r = get_reg(reg_num);
			(*r)++; // Инкрементируем значение регистра
			cores->instruction_pointer++;
		} break;
		case 'D': // Декремент значения регистра
		{
			char reg_num = memory[cores->instruction_pointer];
			reg_t *r = get_reg(reg_num);
			(*r)--; // Декрементируем значение регистра
			cores->instruction_pointer++;
		} break;
		case 'J': // Переход к указанному адресу
		{
			reg_t address = *((reg_t *)(memory + cores->instruction_pointer));
			cores->instruction_pointer += sizeof(reg_t);
			cores->instruction_pointer = address;
		} break;
		case '+': // Прибавление значения к регистру
		{
			char reg1_num = memory[cores->instruction_pointer];
			char num = memory[cores->instruction_pointer + 1];
			reg_t *reg1 = get_reg(reg1_num);
			*reg1 += (short)num - '0';
			cores->instruction_pointer += 2;
		} break;
		case 'A': // Сложение значений двух регистров
		{
			char reg1_num = memory[cores->instruction_pointer];
			char reg2_num = memory[cores->instruction_pointer + 1];
			reg_t *reg1 = get_reg(reg1_num);
			reg_t *reg2 = get_reg(reg2_num);
			*reg1 += *reg2;
			cores->instruction_pointer += 2;
		} break;
		case 'S': // Вычитание значения второго регистра из первого
		{
			char reg1_num = memory[cores->instruction_pointer];
			char reg2_num = memory[cores->instruction_pointer + 1];
			reg_t *reg1 = get_reg(reg1_num);
			reg_t *reg2 = get_reg(reg2_num);
			*reg1 -= *reg2;
			cores->instruction_pointer += 2;
		} break;
		case 'M': // Умножение значений двух регистров
		{
			char reg1_num = memory[cores->instruction_pointer];
			char reg2_num = memory[cores->instruction_pointer + 1];
			reg_t *reg1 = get_reg(reg1_num);
			reg_t *reg2 = get_reg(reg2_num);
			*reg1 *= *reg2;
			cores->instruction_pointer += 2;
		} break;
		case 'L': // Деление значения первого регистра на второй
		{
			char reg1_num = memory[cores->instruction_pointer];
			char reg2_num = memory[cores->instruction_pointer + 1];
			reg_t *reg1 = get_reg(reg1_num);
			reg_t *reg2 = get_reg(reg2_num);
			*reg1 /= *reg2;
			cores->instruction_pointer += 2;
		} break;
		case 'C': // Сравнение значений двух регистров
		{
			char reg1_num = memory[cores->instruction_pointer];
			char reg2_num = memory[cores->instruction_pointer + 1];
			reg_t *reg1 = get_reg(reg1_num);
			reg_t *reg2 = get_reg(reg2_num);
			if (*reg1 < *reg2) {
				cores->instruction_pointer += 3; // Переход к адресу инструкции после следующей
			} else {
				cores->instruction_pointer += sizeof(reg_t) + 1; // Пропуск адреса инструкции после следующей
			}
		} break;
		case 'N': // Установка значения регистра в 0
		{
			char reg_num = memory[cores->instruction_pointer];
			reg_t *r = get_reg(reg_num);
			*r = 0;
			cores->instruction_pointer++;
		} break;
		case 'R': // Копирование значения одного регистра в другой
		{
			char reg1_num = memory[cores->instruction_pointer];
			char reg2_num = memory[cores->instruction_pointer + 1];
			reg_t *reg1 = get_reg(reg1_num);
			reg_t *reg2 = get_reg(reg2_num);
			*reg1 = *reg2;
			cores->instruction_pointer += 2;
		} break;
		default:
			printf("Error! Invalid operation\n");
			exit(-1);
			free(memory);
	}
}

int main(int argc, char **argv) {
	// Открываем файл
	FILE *boot_file = fopen(BOOT_FILE, "rb");
	if (boot_file == NULL) {
		printf("Failed to open boot file: %s\n", BOOT_FILE);
		return 1;
	}

	// Вычисляем размер файла
	fseek(boot_file, 0, SEEK_END);
	long file_size = ftell(boot_file);
	fseek(boot_file, 0, SEEK_SET);

	// Выделяем память для массива
	memory = (char *)malloc(MEM_SIZE);
	if (memory == NULL) {
		printf("Failed to allocate memory for file\n");
		fclose(boot_file);
		return 1;
	}

	// Читаем содержимое файла в массив
	fread(memory, sizeof(char), file_size, boot_file);

	// Закрываем файл, так как содержимое уже в памяти
	fclose(boot_file);

	uint8_t core_id = 0;

	for (uint64_t i = 0; i < THREAD_MAX; i++) {
		avaible_cores[i].state = 0;
		avaible_cores[i].core_id = i;
	}

	avaible_cores[0].state = 1;

	// Выполняем инструкции из массива
	while (1) {
		// printf("core_ID: %u\n", core_id);
		if (running_core < 1) {
			printf("All cores halted!\n");
			exit(0);
		}

		if (core_id >= THREAD_MAX) { core_id = 0; }

		cores = &avaible_cores[core_id];

		if (cores->state != 1) {
			core_id++;
			continue;
		}

		if (memory[cores->instruction_pointer] == ' ' || memory[cores->instruction_pointer] == '\n') {
			cores->instruction_pointer++; // Ничего не делаем
			continue;
		}
		// printf("Executing instruction: %c | %u\n", memory[cores->instruction_pointer], core_id);
		execute_instruction(memory[cores->instruction_pointer]);
		core_id++;
	}

	// Освобождаем память
	free(memory);

	printf("Done\n");
	return 0;
}
