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

    // Заполняем поля задачи
    newTask->priority = priority; 
    newTask->burst = burst;

    // Вставляем задачу в список (insert добавляет в начало)
    insert(&head, newTask);
}

// Основная функция планировщика
void schedule() {
    // Пока в списке есть задачи
    while (head != NULL) {
        // Ищем последний элемент списка (он соответствует первой пришедшей задаче)
        struct node *temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        Task *target = temp->task;
        
        // Выполняем задачу полностью
        run(target, target->burst);
        
        // Удаляем выполненную задачу из списка
        delete(&head, target);
    }
}