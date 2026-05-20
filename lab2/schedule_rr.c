#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "schedulers.h"
#include "list.h"
#include "cpu.h"

// Голова списка задач (глобальная, чтобы add() и schedule() работали с одним списком)
struct node *head = NULL;

// Функция добавления задачи в список
void add(char *name, int priority, int burst) {
    // Выделяем память под новую задачу
    Task *newTask = malloc(sizeof(Task));
    
    // Копируем имя задачи (выделяем отдельную память)
    newTask->name = strdup(name);
    
    // Заполняем остальные поля
    newTask->priority = priority;
    newTask->burst = burst;

    // Вставляем задачу в список (insert добавляет в начало)
    insert(&head, newTask);
}

// Основная функция планировщика
void schedule() {
    // Пока в списке есть задачи
    while (head != NULL) {
        // Берём первую задачу из списка
        struct node *current = head;
        Task *task = current->task;
        
        // Определяем, сколько времени дать задаче в этом кванте
        // Если осталось меньше кванта — выполняем остаток
        int slice = (task->burst < QUANTUM) ? task->burst : QUANTUM;
        
        // Выполняем задачу на slice единиц времени
        run(task, slice);
        
        // Уменьшаем оставшееся время задачи
        task->burst -= slice;
        
        // Удаляем текущий узел из головы списка
        head = head->next;
        
        if (task->burst == 0) {
            // Задача завершилась: освобождаем память
            free(task->name);
            free(task);
            // Узел current больше не нужен, он уже исключён из списка
            free(current);
        } else {
            // Задача не завершилась: перемещаем её в конец очереди
            // Ищем последний элемент списка
            struct node *last = head;
            if (last == NULL) {
                // Список пуст, задача становится единственным элементом
                head = current;
                current->next = NULL;
            } else {
                while (last->next != NULL) {
                    last = last->next;
                }
                // Прикрепляем задачу в конец
                last->next = current;
                current->next = NULL;
            }
        }
    }
}