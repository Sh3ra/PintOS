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

struct semaphore write_syscall_sema,read_syscall_sema;

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

static tid_t execute(char *cmd_line);

static uint32_t tell(int fd);

void syscall_init(void)
{
    lock_init(&filesys_lock);
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
    esp = f->esp;

    if (!is_valid_ptr(esp) || !is_valid_ptr(esp + 1) ||
        !is_valid_ptr(esp + 2) || !is_valid_ptr(esp + 3))
    {
        kill();
    }
    ASSERT(&open_lock != NULL);
    lock_acquire(&open_lock);
    switch (*(int *)f->esp)
    {
    case SYS_HALT:
    {
        //printf("halt\n" );
        shutdown_power_off();
    }
    case SYS_EXIT:
    {
        int status = *((int *)f->esp + 1);
        ourExit(status);
        break;
    }
    case SYS_EXEC:
    {
        char *cmd_line = (char *)(*((int *)f->esp + 1));
        f->eax = execute(cmd_line);
        break;
    }
    case SYS_WAIT:
    {
        tid_t child_pid = (tid_t *)(*((int *)f->esp + 1));
        lock_release(&open_lock);
        f->eax = process_wait(child_pid);
        break;
    }
    case SYS_CREATE:
    {
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
        char *curr_name = (char *)(*((int *)f->esp + 1));
        f->eax = open_file(curr_name);
        break;
    }
    case SYS_FILESIZE:
    {
        int fd = *((int *)f->esp + 1);
        f->eax = filesize(fd);
        break;
    }
    case SYS_READ:
    {
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
        int fd = *((int *)f->esp + 1);
        void *buffer = (void *)(*((int *)f->esp + 2));
        unsigned size = *((unsigned *)f->esp + 3);
        //run the syscall, a function of your own making
        //since this syscall returns a value, the return value should be stored in f->eax
        f->eax = write(fd, buffer, size);
        break;
    }
    case SYS_SEEK:
    {
        int fd = *((int *)f->esp + 1);
        unsigned position = *((unsigned *)f->esp + 2);
        seek(fd, position);
        break;
    }
    case SYS_TELL:
    {
        int fd = *((int *)f->esp + 1);
        f->eax = tell(fd);
        break;
    }
    case SYS_CLOSE:
    {
        int fd = *((int *)f->esp + 1);
        close_file(fd);
        break;
    }
    default:
    {
        kill();
    }
    }
    if(lock_held_by_current_thread(&open_lock))
    lock_release(&open_lock);
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
    return process_execute(cmd_line);
}

static bool create_file(char *curr_name, off_t initial_size)
{
    if (!is_valid_ptr(curr_name))
        kill();
    bool res;
    res = filesys_create(curr_name, initial_size);
    return res;
}

static int remove_file(char *curr_name)
{
    if (!is_valid_ptr(curr_name))
        kill();
    int res;
    res = filesys_remove(curr_name);
    return res;
}

static int open_file(char *curr_name)
{
    if (!is_valid_ptr(curr_name))
        kill();
    int res = -1;
    struct file *curr_file = filesys_open(curr_name);
    if (curr_file != NULL)
    {
        list_push_back(&thread_current()->my_opened_files_list, &curr_file->file_elem);
        thread_current()->fd++;
        curr_file->fd = thread_current()->fd;
        res = thread_current()->fd;
    }
    return res;
}
void
close_all_files(){
    struct list *l = &thread_current()->my_opened_files_list;
    for (struct list_elem * e = list_begin(l); e != list_end(l);) {
        struct file * f = list_entry(e, struct file, file_elem);
        e = list_next(e);
        file_close(f);
    }

}
void ourExit(int status)
{
    printf("%s: exit(%d)\n", thread_current()->name, status);
    struct child_process * cp = thread_current()->cp;
    thread_current()->cp->exit_status = status;
    sema_up(&thread_current()->cp->sema);
    if(thread_current()->my_exec_file != NULL ) {
        file_close(thread_current()->my_exec_file); //close file that was opened in process.c/load function to decrement deny-inode-write again
    }
    close_all_files();
    if(lock_held_by_current_thread(&open_lock)) {
          lock_release(&open_lock);
    }
    thread_exit();
}

static uint32_t write(int fd, void *buffer, unsigned int size) {
    unsigned buffer_size = size;
    void *buffer_tmp = buffer;
    /* check the user memory pointing by buffer are valid */
    while (buffer_tmp != NULL)
    {
        if (!is_valid_ptr(buffer_tmp)) {
            kill();
        }


        /* Advance */
        if (buffer_size > PGSIZE)
        {
            buffer_tmp += PGSIZE;
            buffer_size -= PGSIZE;
        }
        else if (buffer_size == 0)
        {
            /* terminate the checking loop */
            buffer_tmp = NULL;
        }
        else
        {
            /* last loop */
            buffer_tmp = buffer + size - 1;
            buffer_size = 0;
        }
    }
    int res = 0;
    //lock_acquire(&filesys_lock);
    //printf("horray");
    if (fd == 1)
    {
        putbuf(buffer, size);
        res = size;
    }
    else if (fd == 0)
    {
        //if(DEBUG_OMAR)printf("where4\n" );
        res = -1;
    }

    else
    {
        struct file *file = get_file(fd);
        if (file != NULL) {
            //sema_down(&write_syscall_sema);
            //printf("file %d\n" ,file->deny_write);

            res = file_write(file, buffer, size);
            //printf("%d\n",res);
            //sema_up(&write_syscall_sema);
        }
    }
    //lock_release(&filesys_lock);
    return res;
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
    unsigned buffer_size = size;
    void *buffer_tmp = buffer;

    /* check the user memory pointing by buffer are valid */
    while (buffer_tmp != NULL)
    {
        //printf("hey");
        if (!is_valid_ptr(buffer_tmp))
            kill();

        /* Advance */
        if (buffer_size > PGSIZE)
        {
            buffer_tmp += PGSIZE;
            buffer_size -= PGSIZE;
        }
        else if (buffer_size == 0)
        {
            /* terminate the checking loop */
            buffer_tmp = NULL;
        }
        else
        {
            /* last loop */
            buffer_tmp = buffer + size - 1;
            buffer_size = 0;
        }
    }
    if (fd == 0)
    {
        read_helper(buffer, size);
        return size;
    }
    else
    {
        struct file *file = get_file(fd);
        if (file != NULL) {
            sema_down(&read_syscall_sema);
            int result_size = file_read(file, buffer, size);
            sema_up(&read_syscall_sema);
            return result_size ;
        }
        }

    ourExit(-1);
    return -1;
}

uint32_t filesize(int fd)
{
    int res = -1;
    //lock_acquire(&filesys_lock);
    struct file *file = get_file(fd);
    if (file != NULL)
        res = file_length(file);
   // lock_release(&filesys_lock);
    //ourExit(-1);
    return res;
}

static void seek(int fd, unsigned position)
{
    //lock_acquire(&filesys_lock);
    struct file *file = get_file(fd);
    if (file != NULL)
        file_seek(file, position);
   // lock_release(&filesys_lock);
}

static uint32_t tell(int fd)
{
    int res = 0;
    //lock_acquire(&filesys_lock);
    struct file *file = get_file(fd);
    if (file != NULL)
        res = file_tell(file);
    //lock_release(&filesys_lock);
    return res;
}

static void close_file(int fd)
{
    //lock_acquire(&filesys_lock);
    struct file *file = get_file(fd);
    if (file != NULL)
    {
        list_remove(&file->file_elem);
        file_close(file);
    }
    //lock_release(&filesys_lock);
}
