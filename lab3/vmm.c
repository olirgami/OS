#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define PAGE_SIZE 256                // размер страницы 
#define PAGE_TABLE_SIZE 256          // количество записей в таблице страниц
#define TLB_SIZE 16                  // размер TLB 

#define FRAME_SIZE PAGE_SIZE         // размер фрейма = размеру страницы
#define FRAME_COUNT PAGE_TABLE_SIZE  // количество фреймов = количеству страниц

#define PHYSICAL_MEM_SIZE (FRAME_COUNT * FRAME_SIZE)  // 65536 байт

//Маски для битовых операций 
#define MASK_OFFSET 0xFF             // младшие 8 бит (смещение)
#define MASK_PAGE  0xFF00            // старшие 8 бит (номер страницы)

/* Количество бит для сдвига */
#define OFFSET_BITS 8                // смещение занимает 8 бит

// Запись таблицы страниц
typedef struct {
    int frame;      // номер физического фрейма (или -1, если страница не в памяти)
    int present;    // 1 - страница в памяти, 0 - не в памяти
} page_table_entry_t;

// Запись TLB
typedef struct {
    int vpn;        // номер виртуальной страницы
    int frame;      // номер физического фрейма
    int valid;      // 1 - запись действительна, 0 - нет
} tlb_entry_t;

// Таблица страниц (256 записей)
page_table_entry_t page_table[PAGE_TABLE_SIZE];

// TLB (16 записей)
tlb_entry_t tlb[TLB_SIZE];

// Физическая память (массив байтов)
unsigned char physical_memory[PHYSICAL_MEM_SIZE];

// Указатель на файл BACKING_STORE.bin
FILE *backing_store = NULL;

// Статистика
int page_faults = 0;
int tlb_hits = 0;
int total_addresses = 0;

// Для FIFO в TLB
int tlb_next_index = 0;

// Инициализация таблицы страниц (все страницы не в памяти)
void init_page_table() {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i].frame = -1;
        page_table[i].present = 0;
    }
}

// Инициализация TLB (все записи невалидны)
void init_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = 0;
    }
}

// Инициализация физической памяти (заполняем нулями)
void init_physical_memory() {
    memset(physical_memory, 0, PHYSICAL_MEM_SIZE);
}

// Разбиение виртуального адреса на VPN и offset
void split_address(int virtual_address, int *vpn, int *offset) {
    *vpn = (virtual_address & MASK_PAGE) >> OFFSET_BITS;
    *offset = virtual_address & MASK_OFFSET;
}

// Поиск в TLB по VPN
// Возвращает номер фрейма, если найден, иначе -1
int lookup_tlb(int vpn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            tlb_hits++;
            return tlb[i].frame;
        }
    }
    return -1;
}

// Добавление записи в TLB (FIFO)
void add_to_tlb(int vpn, int frame) {
    tlb[tlb_next_index].vpn = vpn;
    tlb[tlb_next_index].frame = frame;
    tlb[tlb_next_index].valid = 1;
    
    tlb_next_index = (tlb_next_index + 1) % TLB_SIZE;
}

// Загрузка страницы из BACKING_STORE.bin в указанный фрейм
void load_page_from_backing_store(int page_num, int frame_num) {
    // Вычисляем смещение в файле: номер страницы * размер страницы
    int offset_in_file = page_num * PAGE_SIZE;
    
    // Вычисляем адрес в физической памяти: номер фрейма * размер фрейма
    int address_in_memory = frame_num * PAGE_SIZE;
    
    // Перемещаем указатель в файле на нужную позицию
    fseek(backing_store, offset_in_file, SEEK_SET);
    
    // Читаем PAGE_SIZE байт из файла в физическую память
    size_t bytes_read = fread(&physical_memory[address_in_memory], 1, PAGE_SIZE, backing_store);
    
    if (bytes_read != PAGE_SIZE) {
        fprintf(stderr, "Ошибка чтения BACKING_STORE.bin: страница %d\n", page_num);
    }
}

// Обработка page fault: загружаем страницу в свободный фрейм
int handle_page_fault(int page_num) {
    int frame_num = -1;
    
    // Ищем свободный фрейм (где нет страницы)
    for (int i = 0; i < FRAME_COUNT; i++) {
        int used = 0;
        for (int j = 0; j < PAGE_TABLE_SIZE; j++) {
            if (page_table[j].present && page_table[j].frame == i) {
                used = 1;
                break;
            }
        }
        if (!used) {
            frame_num = i;
            break;
        }
    }
    
    
    
    // Загружаем страницу в выбранный фрейм
    load_page_from_backing_store(page_num, frame_num);
    
    // Обновляем таблицу страниц
    page_table[page_num].frame = frame_num;
    page_table[page_num].present = 1;
    
    return frame_num;
}

// Трансляция виртуального адреса в физический
int translate_address(int virtual_address) {
    int vpn, offset;
    split_address(virtual_address, &vpn, &offset);
    
    // Проверяем TLB
    int frame = lookup_tlb(vpn);
    
    if (frame != -1) {
        // TLB hit
        return frame * PAGE_SIZE + offset;
    }
    
    // TLB miss — идём в таблицу страниц
    if (page_table[vpn].present) {
        // Страница в памяти
        frame = page_table[vpn].frame;
        // Добавляем в TLB
        add_to_tlb(vpn, frame);
        return frame * PAGE_SIZE + offset;
    } else {
        // Page fault — страница не в памяти
        page_faults++;
        frame = handle_page_fault(vpn);
        if (frame == -1) {
            fprintf(stderr, "Ошибка: не удалось загрузить страницу %d\n", vpn);
            return -1;
        }
        // Добавляем в TLB
        add_to_tlb(vpn, frame);
        return frame * PAGE_SIZE + offset;
    }
}

int main(int argc, char *argv[]) {
    // Проверка аргументов командной строки
    if (argc != 2) {
        fprintf(stderr, "Использование: %s addresses.txt\n", argv[0]);
        exit(1);
    }
    
    // Открываем BACKING_STORE.bin
    backing_store = fopen("BACKING_STORE.bin", "rb");
    if (backing_store == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть BACKING_STORE.bin\n");
        exit(1);
    }
    
    // Открываем файл с адресами
    FILE *address_file = fopen(argv[1], "r");
    if (address_file == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть файл %s\n", argv[1]);
        fclose(backing_store);
        exit(1);
    }
    
    // Инициализация
    init_page_table();
    init_tlb();
    init_physical_memory();
    
    // Сброс счётчиков
    tlb_next_index = 0;
    page_faults = 0;
    tlb_hits = 0;
    total_addresses = 0;
    
    // Чтение адресов и трансляция
    int virtual_address;
    while (fscanf(address_file, "%d", &virtual_address) != EOF) {
        total_addresses++;
        
        // Транслируем адрес
        int physical_address = translate_address(virtual_address);
        if (physical_address == -1) {
            fprintf(stderr, "Ошибка трансляции адреса %d\n", virtual_address);
            continue;
        }
        
        // Читаем байт из физической памяти
        signed char value = physical_memory[physical_address];
        
        // Выводим результат
        printf("Virtual address: %d Physical address: %d Value: %d\n", 
               virtual_address, physical_address, value);
    }
    
    // Выводим статистику
    printf("\nСТАТИСТИКА\n");
    printf("Всего обращений: %d\n", total_addresses);
    printf("Попаданий в TLB: %d (%.2f%%)\n", tlb_hits, 
           total_addresses > 0 ? (float)tlb_hits / total_addresses * 100 : 0);
    printf("Page faults: %d (%.2f%%)\n", page_faults, 
           total_addresses > 0 ? (float)page_faults / total_addresses * 100 : 0);
    
    // Закрываем файлы
    fclose(address_file);
    fclose(backing_store);
    
    return 0;
}