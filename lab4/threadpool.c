#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "threadpool.h"

// определяем Task здесь
typedef struct task {
    void (*function)(void *);   // указатель на функцию, которую нужно выполнить
    void *arg;                  // аргумент для этой функции
} Task;

// Один узел очереди: хранит указатель на задачу и на следующий узел
typedef struct node {
    Task *task;          // сама задача (функция + аргумент)
    struct node *next;   // ссылка на следующую задачу в очереди
} node_t;

// Голова и хвост очереди задач 
static node_t *head = NULL;
static node_t *tail = NULL;

// Массив идентификаторов потоков и их количество
static pthread_t *workers = NULL;
static int num_workers = 0;

// Флаг, который показывает, что пул завершает работу.
// Нужен, чтобы потоки могли выйти из бесконечного цикла.
static int shutdown_flag = 0;

// Счётчик задач в очереди (для отладки, не обязателен)
static int tasks_count = 0;

// Мьютекс защищает очередь от одновременного доступа нескольких потоков.
// Без него два потока могли бы взять одну задачу или сломать указатели.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Семафор — это счётчик доступных задач.
// Когда семафор = 0, потоки спят. Когда > 0, они просыпаются.
static sem_t sem;


// Добавление задачи в конец очереди
void enqueue(Task *task) {
    // Создаём новый узел
    node_t *new_node = malloc(sizeof(node_t));
    new_node->task = task;
    new_node->next = NULL;

    // Если очередь была пуста, новый узел становится и головой, и хвостом
    if (tail == NULL) {
        head = tail = new_node;
    } else {
        // Иначе прицепляем новый узел после текущего хвоста
        tail->next = new_node;
        tail = new_node;   // хвост сдвигается на новый узел
    }
    tasks_count++;
}

// Извлечение задачи из начала очереди 
Task *dequeue() {
    if (head == NULL) return NULL;   // очередь пуста

    node_t *temp = head;        // запоминаем старую голову
    Task *task = head->task;    // забираем задачу из головы
    head = head->next;          // сдвигаем голову на следующий узел

    // если очередь стала пустой, хвост тоже должен указывать в никуда
    if (head == NULL) tail = NULL;

    free(temp);                 // удаляем старый узел
    tasks_count--;
    return task;                // возвращаем задачу
}


// Эта функция выполняется каждым потоком из пула.
// Поток работает в бесконечном цикле: ждёт задачу, выполняет её, и так далее.
void *worker(void *arg) {
    (void)arg; // параметр не используется, но компилятор не ругается

    while (1) {
        // Ждём сигнал от семафора.
        // Если семафор = 0, поток засыпает.
        // Если семафор > 0, уменьшаем его на 1 и идём дальше.
        sem_wait(&sem);

        // Захватываем мьютекс — теперь только один поток может трогать очередь.
        pthread_mutex_lock(&mutex);

        // Проверяем: если пул завершается И очередь пуста, пора выходить.
        if (shutdown_flag && head == NULL) {
            pthread_mutex_unlock(&mutex);
            break;   // выход из бесконечного цикла → поток завершится
        }

        // Берём задачу из очереди (голова)
        Task *task = dequeue();
        pthread_mutex_unlock(&mutex);   // отпускаем мьютекс

        // Если задачу получили — выполняем её
        if (task != NULL) {
            task->function(task->arg);  // вызываем функцию из задачи
            free(task);                          // задача больше не нужна
        }
        // Если задача == NULL, значит, нас разбудили без работы
        // Тогда мы просто вернёмся к началу цикла и на следующей итерации
        // увидим shutdown_flag и выйдем.
    }
    return NULL;
}


// Инициализация пула потоков.
int pool_init(int num_threads) {
    num_workers = num_threads;
    shutdown_flag = 0;
    tasks_count = 0;
    head = tail = NULL;   // очередь пуста

    // Семафор инициализируем нулём.
    // Начальное значение 0 означает: сначала работы нет.
    // Второй параметр 0 — семафор используется только между потоками одного процесса.
    sem_init(&sem, 0, 0);

    // Выделяем память под массив идентификаторов потоков
    workers = malloc(num_workers * sizeof(pthread_t));
    if (workers == NULL) return -1;

    // Создаём потоки. Каждый поток сразу уходит в worker() и засыпает на sem_wait().
    for (int i = 0; i < num_workers; i++) {
        if (pthread_create(&workers[i], NULL, worker, NULL) != 0) {
            return -1;   // если не удалось создать хотя бы один поток — ошибка
        }
    }
    return 0;
}

// Отправка задачи в пул.
int pool_submit(void (*function)(void *), void *arg) {
    // Создаём структуру задачи
    Task *task = (Task *)malloc(sizeof(Task));
    if (task == NULL) return 1;   // не хватило памяти

    task->function = function;
    task->arg = arg;

    // Захватываем мьютекс, чтобы безопасно добавить задачу в очередь
    pthread_mutex_lock(&mutex);
    enqueue(task);
    pthread_mutex_unlock(&mutex);

    // Увеличиваем семафор на 1 и будим один спящий поток.
    // Если потоков больше, чем задач, разбудится только один.
    sem_post(&sem);

    return 0;
}

// Завершение работы пула.
void pool_shutdown(void) {
    // Устанавливаем флаг завершения под защитой мьютекса,
    // чтобы поток не увидел флаг в момент, когда он ещё не установлен.
    pthread_mutex_lock(&mutex);
    shutdown_flag = 1;
    pthread_mutex_unlock(&mutex);

    // Будим все потоки, чтобы они могли выйти из sem_wait().
    // Каждый поток увидит shutdown_flag и, если очередь пуста, выйдет.
    for (int i = 0; i < num_workers; i++) {
        sem_post(&sem);
    }

    // Ждём завершения каждого потока (pthread_join блокирует главный поток).
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }

    // Освобождаем ресурсы
    free(workers);
    sem_destroy(&sem);
    pthread_mutex_destroy(&mutex);

    // Очищаем очередь на случай, если в ней остались невыполненные задачи
    while (head != NULL) {
        node_t *temp = head;
        head = head->next;
        free(temp->task);   // сама задача
        free(temp);         // узел очереди
    }
    tail = NULL;
    tasks_count = 0;
}
