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
        // 1. Находим самый высокий приоритет среди оставшихся задач
        struct node *temp = head;
        int max_prio = MIN_PRIORITY;
        while (temp != NULL) {
            if (temp->task->priority > max_prio) {
                max_prio = temp->task->priority;
            }
            temp = temp->next;
        }

        // Один проход Round Robin по всем задачам с найденным максимальным приоритетом
        struct node *current = head;
        while (current != NULL) {
            // Сохраняем следующий элемент, потому что текущий может быть удалён
            struct node *next_node = current->next;
            Task *t = current->task;

            // Обрабатываем только задачи с максимальным приоритетом
            if (t->priority == max_prio) {
                // Определяем, сколько времени дать задаче в этом кванте
                int slice = (t->burst > QUANTUM) ? QUANTUM : t->burst;
                
                // Выполняем задачу на slice единиц времени
                run(t, slice);
                
                // Уменьшаем оставшееся время задачи
                t->burst -= slice;

                // Если задача завершилась
                if (t->burst == 0) {
                    // Удаляем её из списка
                    delete(&head, t);
                    // Освобождаем память
                    free(t->name);
                    free(t);
                }
            }
            // Переходим к следующему элементу
            current = next_node;
        }
    }
}