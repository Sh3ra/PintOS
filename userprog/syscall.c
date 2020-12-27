#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <devices/input.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "filesys/off_t.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "process.h"
#include "filesys/file.h"

struct semaphore write_syscall_sema;
struct semaphore read_syscall_sema;

static void syscall_handler(struct intr_frame *);

static uint32_t write(int fd, void *pVoid, unsigned int size);

static uint32_t read(int fd, void *buffer, unsigned size);

static uint32_t filesize(int fd);

void ourExit(int status);

static int open_file(char *curr_name);

static void close_file(int fd);

static bool create_file(char *curr_name, off_t initial_size);

static int remove_file(char *curr_name);

static void seek(int fd, unsigned position);

static int wait(tid_t pid);

static tid_t execute(char *cmd_line);

void
syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    sema_init(&write_syscall_sema, 1);
    sema_init(&read_syscall_sema, 1);
}

static void
syscall_handler(struct intr_frame *f UNUSED) {
    if (f->esp == NULL || pagedir_get_page(thread_current()->pagedir, f->esp) == NULL) {
        ourExit(-1);
        //ToDo SYS_EXIT
        // يحيى و شعراوي بيضربوا بعض
    }

    switch (*(int *) f->esp) {
        case SYS_HALT: {
            shutdown_power_off();
            break;
        }
        case SYS_EXIT: {
            int status = *((int *) f->esp + 1);
            ourExit(status);
            break;
        }
        case SYS_EXEC: {
            char *cmd_line = (char *) (*((int *) f->esp + 1));
            if (cmd_line == NULL) {
                ourExit(-1);
            }
            f->eax = execute(cmd_line);
            break;
        }
        case SYS_WAIT: {
            tid_t child_pid = (tid_t *) (*((int *) f->esp + 1));
            f->eax = wait(child_pid);
            break;
        }
        case SYS_CREATE: {

            char *curr_name = (char *) (*((int *) f->esp + 1));
            if (curr_name == NULL) {
                ourExit(-1);
            }
            off_t initial_size = (off_t *) (*((int *) f->esp + 2));
            f->eax = create_file(curr_name, initial_size);
            break;
        }
        case SYS_REMOVE: {
            char *curr_name = (char *) (*((int *) f->esp + 1));
            if (curr_name == NULL) {
                ourExit(-1);
            }
            if (curr_name == NULL) {
                ourExit(-1);
            }
            f->eax = remove_file(curr_name);
            break;
        }
        case SYS_OPEN: {
            char *curr_name = (char *) (*((int *) f->esp + 1));
            if (curr_name == NULL) {
                f->eax = -1;
                return;
            }
            f->eax = open_file(curr_name);
            break;
        }
        case SYS_FILESIZE: {
            int fd = *((int *) f->esp + 1);
            f->eax = filesize(fd);
            break;
        }
        case SYS_READ: {
            int fd = *((int *) f->esp + 1);
            void *buffer = (void *) (*((int *) f->esp + 2));
            unsigned size = *((unsigned *) f->esp + 3);
            //run the syscall, a function of your own making
            //since this syscall returns a value, the return value should be stored in f->eax
            f->eax = read(fd, buffer, size);
            //printf("wanted %d read %d\n",size,f->eax);
            break;
        }
        case SYS_WRITE: {
            int fd = *((int *) f->esp + 1);
            void *buffer = (void *) (*((int *) f->esp + 2));
            unsigned size = *((unsigned *) f->esp + 3);
            //run the syscall, a function of your own making
            //since this syscall returns a value, the return value should be stored in f->eax
            f->eax = write(fd, buffer, size);
            break;
        }
        case SYS_SEEK: {
            int fd = *((int *) f->esp + 1);
            unsigned position = *((unsigned *) f->esp + 2);
            seek(fd, position);
            break;
        }
        case SYS_TELL: {
            break;
        }
        case SYS_CLOSE: {
            int fd = *((int *) f->esp + 1);
            close_file(fd);
            break;
        }
        default: {
            thread_exit();
        }
    }
}


struct file *get_file(int fd) {
    struct list_elem *file_elem = list_front(&thread_current()->my_opened_files_list);
    while (list_entry(file_elem, struct file, file_elem)->fd < fd &&
           file_elem != list_tail(&thread_current()->my_opened_files_list)) {
        file_elem = file_elem->next;
    }
    if (file_elem != list_tail(&thread_current()->my_opened_files_list))
        return list_entry(file_elem, struct file, file_elem);
    else return NULL;

}

static tid_t execute(char *cmd_line) {
    tid_t pid = process_execute(cmd_line);
    struct thread *t = get_process_with_specific_tid(pid);
    if (t == NULL) return -1;
    list_push_back(&thread_current()->children_list, &t->child_list_elem);
    return pid;
}

static int wait(tid_t pid) {
    return process_wait(pid);
}

static bool create_file(char *curr_name, off_t initial_size) {
    return filesys_create(curr_name, initial_size);
}

static int remove_file(char *curr_name) {
    return filesys_remove(curr_name);
}

static int open_file(char *curr_name) {
    struct file *curr_file = filesys_open(curr_name);
    list_push_back(&thread_current()->my_opened_files_list, &curr_file->file_elem);
    if (curr_file != NULL) {
        thread_current()->fd++;
        curr_file->fd = thread_current()->fd;
        return thread_current()->fd;
    } else {
        return -1;
    }
}

void ourExit(int status) {
    printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_current()->parent->last_child_status = status;
    thread_exit();
}

static uint32_t write(int fd, void *buffer, unsigned int size) {
    if (fd == 1) {
        putbuf(buffer, size);
        return size;
    } else if (fd == NULL || fd == 0 || list_empty(&thread_current()->my_opened_files_list))return 0;
    else {
        struct file *file = get_file(fd);
        if (file != NULL)return file_write(file, buffer, size);
    }
    return 0;
}

static void read_helper(void *buffer, unsigned int size) {
    while (size-- > 0) {
        *(uint8_t *) buffer = input_getc();
        buffer++;
    }
}

static uint32_t read(int fd, void *buffer, unsigned size) {
    if (fd == 0) {
        read_helper(buffer, size);
        return size;
    } else if (fd == 1 || list_empty(&thread_current()->my_opened_files_list));
    else {
        struct file *file = get_file(fd);
        if (file != NULL)
            return file_read(file, buffer, size);
    }
    ourExit(-1);
    return -1;
}

uint32_t filesize(int fd) {
    return 0 ;
}

static void seek(int fd, unsigned position) {
    if (fd == NULL || fd < 2 || list_empty(&thread_current()->my_opened_files_list))return;
    struct file *file = get_file(fd);
    if (file != NULL)
        file_seek(file, position);
}

static void close_file(int fd) {
    if (fd < 2 || list_empty(&thread_current()->my_opened_files_list))return;
    struct file *file = get_file(fd);
    if (file != NULL) {
        list_remove(&file->file_elem);
        file_close(file);
    }
}
