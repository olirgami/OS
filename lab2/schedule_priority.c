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
    
    // Копируем имя задачи (выделяем отдельную памят)
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
        // Начинаем поиск задачи с наивысшим приоритетом с первого элемента
        struct node *current = head;
        Task *highest = current->task;
        
        // Проходим по всему списку, ищем задачу с наибольшим priority
        while (current != NULL) {
            if (current->task->priority > highest->priority) {
                highest = current->task;
            }
            current = current->next;
        }
        
        // Выполняем найденную задачу полностью
        run(highest, highest->burst);
        
        // Удаляем выполненную задачу из списка
        delete(&head, highest);
        
        // Освобождаем память, выделенную под имя задачи и под саму задачу
        free(highest->name);
        free(highest);
    }
}