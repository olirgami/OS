#include "myshell.h"

char shell_path[MAX_PATH];

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");
    
    FILE* input_stream = stdin; // По умолчанию читаем с клавиатуры
    
    // Определяем полный путь к оболочке для переменной окружения
    if (realpath(argv[0], shell_path) == NULL) {
        perror("Ошибка при определении пути realpath");
        strncpy(shell_path, argv[0], MAX_PATH); 
        shell_path[MAX_PATH - 1] = '\0';
    }
    
    // Установка переменной shell
    setenv("shell", shell_path, 1);

    // Если передан аргумент, переключаемся на пакетный режим 
    if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            fprintf(stderr, "Ошибка: не удалось открыть файл '%s'\n", argv[1]);
            return 1;
        }
    } 
    else if (argc > 2) {
        fprintf(stderr, "Ошибка: слишком много аргументов '%s'\n", argv[0]);
        return 1;
    }

    // Запуск основного цикла работы
    shell_loop(input_stream);

    // Закрываем файл, если работали в пакетном режиме
    if (input_stream != stdin) {
        fclose(input_stream);
    }

    return 0;
}


void shell_loop(FILE* input_src) {
    char line[MAX_PATH];
    char** args;

    while (1) {
        // Печатаем приглашение только в интерактивном режиме
        if (input_src == stdin) {
            printf("myshell> ");
            fflush(stdout);
        }

        // Читаем строку, если конец файла — выходим
        if (fgets(line, MAX_PATH, input_src) == NULL) {
            break; 
        }

        // В пакетном режиме дублируем считанную команду на экран
        if (input_src != stdin) {
            printf("myshell> %s", line);
        }

        // Убираем символ новой строки в конце
        line[strcspn(line, "\n")] = 0;

        // Разбиваем строку на аргументы
        args = split_line(line);

        if (args != NULL && args[0] != NULL) {
            if (strcmp(args[0], "quit") == 0) {
                free_args(args);
                break;
            }
            // Выполняем команду через диспетчер
            run_command(args);
        }

        // Освобождаем память после каждой команды
        free_args(args);
    }
}