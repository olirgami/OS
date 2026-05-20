#include "myshell.h"

// Делаем путь к шеллу доступным для установки переменной parent
extern char shell_path[MAX_PATH];

// Функция для разбиения строки на отдельные слова (аргументы)
char** split_line(char* line) {
    int bufsize = MAX_ARGS;
    int position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* token;

    if (!tokens) {
        fprintf(stderr, "myshell: ошибка malloc\n");
        exit(1);
    }

    // Разделяем строку по пробелам, табуляциям и переносам строк
    token = strtok(line, " \t\r\n\a");
    while (token != NULL) {
        // Копируем каждое слово в новую область памяти
        tokens[position] = malloc(strlen(token) + 1);
        if (tokens[position]) {
            strcpy(tokens[position], token);
        }
        position++;

        if (position >= bufsize) break;
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

// Освобождение памяти, выделенной под массив аргументов
void free_args(char** args) {
    if (args == NULL) return;
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

// Внутренняя команда смены директории
int command_cd(char** args) {
    if (args[1] == NULL) {
        char cwd[MAX_PATH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
    } else {
        if (chdir(args[1]) != 0) {
            perror("myshell: cd");
        } else {
            char cwd[MAX_PATH];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                setenv("PWD", cwd, 1);
            }
        }
    }
    return 0;
}

// Очистка экрана терминала
int command_clr(void) {
    printf("\033[H\033[J");
    return 0;
}

// Просмотр содержимого каталога
int command_dir(char** args) {
    const char* path = (args[1] == NULL) ? "." : args[1];
    DIR* d = opendir(path);
    if (d == NULL) {
        perror("myshell: dir");
        return 1;
    }
    struct dirent* dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] != '.') {
            printf("%s\n", dir->d_name);
        }
    }
    closedir(d);
    return 0;
}

// Вывод текста пользователя на экран
int command_echo(char** args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s%s", args[i], (args[i+1] != NULL) ? " " : "");
    }
    printf("\n");
    return 0;
}

// Вывод руководства пользователя из внешнего файла
int command_help(void) {
    printf("\n Справка по myshell \n");
    printf("cd [путь]    - Сменить директорию (без пути - показать текущую)\n");
    printf("clr          - Очистить экран\n");
    printf("dir [путь]   - Показать содержимое папки\n");
    printf("environ      - Показать переменные окружения\n");
    printf("echo [текст] - Вывести текст на экран\n");
    printf("pause        - Заморозить оболочку до нажатия Enter\n");
    printf("help         - Показать это сообщение\n");
    printf("quit         - Выйти из программы\n\n");
    return 0;
}

// Приостановка работы интерпретатора
int command_pause(void) {
    printf("Нажмите Enter, чтобы продолжить");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    return 0;
}

// Запуск внешних программ
int run_external(char** args) {
    pid_t pid = fork(); // Создание дочернего процесса  
    if (pid == 0) {
        setenv("parent", shell_path, 1);
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "myshell: команда не найдена: %s\n", args[0]);
        }
        _exit(1);
    } else if (pid < 0) {
        perror("myshell: fork");
        return 1;
    } else {
        // Родительский процесс ждет завершения работы программы-потомка
        waitpid(pid, NULL, 0);
    }
    return 0;
}

// Диспетчер команд: выбирает, какую функцию запустить
void run_command(char** args) {
    if (args == NULL || args[0] == NULL) return;

    if (strcmp(args[0], "cd") == 0) command_cd(args);
    else if (strcmp(args[0], "clr") == 0) command_clr();
    else if (strcmp(args[0], "dir") == 0) command_dir(args);
    else if (strcmp(args[0], "environ") == 0) {
        extern char** environ;
        for (int i = 0; environ[i] != NULL; i++) printf("%s\n", environ[i]);
    }
    else if (strcmp(args[0], "echo") == 0) command_echo(args);
    else if (strcmp(args[0], "help") == 0) command_help();
    else if (strcmp(args[0], "pause") == 0) command_pause();
    else run_external(args);
}