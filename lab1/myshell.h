
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

// Основные константы для размеров путей и аргументов
#define MAX_PATH 1024
#define MAX_ARGS 64
#define MAX_LINE 1024

// Путь к исполняемому файлу шелла
extern char shell_path[MAX_PATH];

// Функции для работы со строками и памятью
char** split_line(char* line);
void free_args(char** args);

// Внутренние команды
int command_cd(char** args);
int command_clr(void);
int command_dir(char** args);
int command_echo(char** args);
int command_environ(void);
int command_help(void);
int command_pause(void);

// Запуск внешних программ и диспетчер выбора команд
int run_external(char** args);
void run_command(char** args);

// Главный цикл обработки ввода
void shell_loop(FILE* input_src);