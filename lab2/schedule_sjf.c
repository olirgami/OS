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
        // Начинаем поиск самой короткой задачи с первого элемента
        struct node *current = head;
        Task *shortest = current->task;
        
        // Проходим по всему списку, ищем задачу с наименьшим burst
        while (current != NULL) {
            if (current->task->burst < shortest->burst) {
                shortest = current->task;
            }
            current = current->next;
        }
        
        // Выполняем найденную задачу полностью
        run(shortest, shortest->burst);
        
        // Удаляем выполненную задачу из списка
        delete(&head, shortest);
        
        // Освобождаем память, выделенную под имя задачи и под саму задачу
        free(shortest->name);
        free(shortest);
    }
}