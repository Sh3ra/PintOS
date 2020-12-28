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
#include "threads/vaddr.h"

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

static uint32_t tell(int fd);

void syscall_init(void)
{
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    sema_init(&write_syscall_sema, 1);
    sema_init(&read_syscall_sema, 1);
}

static bool
is_valid_uvaddr(const void *uvaddr)
{
    return (uvaddr != NULL && is_user_vaddr(uvaddr));
}
/* Checks for validity of a user address
   It should be below PHYS_BASE and
   registered in page directory */
bool is_valid_ptr(const void *usr_ptr)
{
    struct thread *cur = thread_current();
    if (is_valid_uvaddr(usr_ptr))
    {
        return (pagedir_get_page(cur->pagedir, usr_ptr)) != NULL;
    }
    return false;
}

/* Exits the process with -1 status */
static void kill()
{
    ourExit(-1);
}
static uint32_t *esp;
static void
syscall_handler(struct intr_frame *f UNUSED)
{
    //printf("syscall\n");
    esp = f->esp;

    if (!is_valid_ptr(esp) || !is_valid_ptr(esp + 1) ||
        !is_valid_ptr(esp + 2) || !is_valid_ptr(esp + 3))
    {
        kill();
    }
    switch (*(int *)f->esp)
    {
    case SYS_HALT:
    {
        //printf("halt\n");
        shutdown_power_off();
    }
    case SYS_EXIT:
    {
        //printf("exit\n");
        int status = *((int *)f->esp + 1);
        ourExit(status);
        break;
    }
    case SYS_EXEC:
    {
        //printf("exec\n");
        char *cmd_line = (char *)(*((int *)f->esp + 1));
        f->eax = execute(cmd_line);
        break;
    }
    case SYS_WAIT:
    {
        //printf("wait\n");
        tid_t child_pid = (tid_t *)(*((int *)f->esp + 1));
        f->eax = wait(child_pid);
        break;
    }
    case SYS_CREATE:
    {
        // printf("create\n");
        char *curr_name = (char *)(*((int *)f->esp + 1));
        if (curr_name == NULL)
        {
            ourExit(-1);
        }
        off_t initial_size = (off_t *)(*((int *)f->esp + 2));
        f->eax = create_file(curr_name, initial_size);
        break;
    }
    case SYS_REMOVE:
    {
        //printf("remove\n");
        char *curr_name = (char *)(*((int *)f->esp + 1));
        if (curr_name == NULL)
        {
            ourExit(-1);
        }

        f->eax = remove_file(curr_name);
        break;
    }
    case SYS_OPEN:
    {
        //printf("open\n");
        char *curr_name = (char *)(*((int *)f->esp + 1));
        if (curr_name == NULL)
        {
            f->eax = -1;
            return;
        }
        f->eax = open_file(curr_name);
        break;
    }
    case SYS_FILESIZE:
    {
        //printf("filesize\n");
        int fd = *((int *)f->esp + 1);
        f->eax = filesize(fd);
        break;
    }
    case SYS_READ:
    {
        //printf("read\n");
        int fd = *((int *)f->esp + 1);
        void *buffer = (void *)(*((int *)f->esp + 2));
        unsigned size = *((unsigned *)f->esp + 3);
        //run the syscall, a function of your own making
        //since this syscall returns a value, the return value should be stored in f->eax
        f->eax = read(fd, buffer, size);
        //printf("wanted %d read %d\n",size,f->eax);
        break;
    }
    case SYS_WRITE:
    {
        //printf("write\n");
        int fd = *((int *)f->esp + 1);
        void *buffer = (void *)(*((int *)f->esp + 2));
        unsigned size = *((unsigned *)f->esp + 3);
        //run the syscall, a function of your own making
        //since this syscall returns a value, the return value should be stored in f->eax
        f->eax = write(fd, buffer, size);
        break;
        f->eax = write(fd, buffer, size);
    }
    case SYS_SEEK:
    {
        // printf("seek\n");
        int fd = *((int *)f->esp + 1);
        unsigned position = *((unsigned *)f->esp + 2);
        seek(fd, position);
        break;
    }
    case SYS_TELL:
    {
        //printf("tell\n");
        int fd = *((int *) f->esp + 1);
        f->eax = tell(fd);
        break;
    }
    case SYS_CLOSE:
    {
        //printf("close\n");
        int fd = *((int *)f->esp + 1);
        close_file(fd);
        break;
    }
    default:
    {
        //printf("default\n");
        kill();
    }
    }
}

struct file *get_file(int fd)
{
    struct file *file = NULL;
    if (fd == NULL || list_empty(&thread_current()->my_opened_files_list))
        return file;
    struct list_elem *file_elem = list_front(&thread_current()->my_opened_files_list);
    while (file_elem != list_tail(&thread_current()->my_opened_files_list) &&
           (file = list_entry(file_elem, struct file, file_elem))->fd < fd)
    {
        file_elem = file_elem->next;
    }
    if (file_elem != list_tail(&thread_current()->my_opened_files_list) && file->fd == fd)
        return list_entry(file_elem, struct file, file_elem);
    else
        return NULL;
}

static tid_t execute(char *cmd_line)
{
    if (!is_valid_ptr(cmd_line))
    {
        kill();
    }
    tid_t pid = process_execute(cmd_line);
    struct thread *t = get_process_with_specific_tid(pid);
    if (t == NULL)
        return -1;
    list_push_back(&thread_current()->children_list, &t->child_list_elem);
    return pid;
}

static int wait(tid_t pid)
{
    return process_wait(pid);
}

static bool create_file(char *curr_name, off_t initial_size)
{
    if (!is_valid_ptr(curr_name))
        kill();
    return filesys_create(curr_name, initial_size);
}

static int remove_file(char *curr_name)
{
    if (!is_valid_ptr(curr_name))
        kill();
    return filesys_remove(curr_name);
}

static int open_file(char *curr_name)
{
    if (!is_valid_ptr(curr_name))
        kill();
    struct file *curr_file = filesys_open(curr_name);
    if (curr_file != NULL)
    {
        list_push_back(&thread_current()->my_opened_files_list, &curr_file->file_elem);
        thread_current()->fd++;
        curr_file->fd = thread_current()->fd;
        return thread_current()->fd;
    }
    else
    {
        return -1;
    }
}

void ourExit(int status)
{
    printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_current()->parent->last_child_status = status;
    thread_exit();
}

static uint32_t write(int fd, void *buffer, unsigned int size)
{
    if (!is_valid_ptr(buffer))
        kill();
    if (fd == 1)
    {
        putbuf(buffer, size);
        return size;
    }
    else
    {
        struct file *file = get_file(fd);
        if (file != NULL)
            return file_write(file, buffer, size);
    }
    return 0;
}

static void read_helper(void *buffer, unsigned int size)
{
    while (size-- > 0)
    {
        *(uint8_t *)buffer = input_getc();
        buffer++;
    }
}

static uint32_t read(int fd, void *buffer, unsigned size)
{
    if (!is_valid_ptr(buffer))
        kill();
    if (fd == 0)
    {
        read_helper(buffer, size);
        return size;
    }
    else
    {
        struct file *file = get_file(fd);
        if (file != NULL)
            return file_read(file, buffer, size);
    }
    ourExit(-1);
    return -1;
}

uint32_t filesize(int fd)
{
    struct file *file = get_file(fd);
    if (file != NULL)
        return file_length(file);
    ourExit(-1);
}

static void seek(int fd, unsigned position)
{
    struct file *file = get_file(fd);
    if (file != NULL)
        file_seek(file, position);
    else
        ourExit(-1);
}

static uint32_t tell(int fd) {
    struct file *file = get_file(fd);
    if (file != NULL)
        return file_tell(file);
    else ourExit(-1);
}

static void close_file(int fd)
{
    struct file *file = get_file(fd);
    if (file != NULL)
    {
        list_remove(&file->file_elem);
        file_close(file);
    }
}
