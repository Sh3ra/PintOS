#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "filesys/off_t.h"

static void syscall_handler(struct intr_frame *);

static uint32_t write(int fd, void *pVoid, unsigned int size);

static void ourExit(int status);

static int open_file(char * curr_name);

static int create_file(char * curr_name, off_t initial_size);

static int remove_file(char * curr_name);

static int wait(tid_t pid);

static tid_t execute(char * cmd_line);

void
syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED) {

    if (f->esp == NULL) {
        thread_exit();
        //ToDo SYS_EXIT
        // يحيى و شعراوي بيضربوا بعض
    }
    switch (*(int *) f->esp) {
        case SYS_HALT: {
            shutdown_power_off();
            break;
        }
        case SYS_EXIT: {
            int status = *((int*)f->esp + 1);
            ourExit(status);
            break;
        }
        case SYS_EXEC: {
            char* cmd_line = (char*)(*((int*)f->esp + 1));
            if(cmd_line == NULL) {
              ourExit(-1);
            }
            //f->eax = execute(cmd_line);
            break;
        }
        case SYS_WAIT: {
            tid_t child_pid = (tid_t*)(*((int*)f->esp + 1));
            //f->eax = wait(child_pid);
            break;
        }
        case SYS_CREATE: {
            char * curr_name = (char*)(*((int*)f->esp + 1));
            if(curr_name == NULL) {
              ourExit(-1);
            }
            off_t initial_size = (off_t*)(*((int*)f->esp + 2));
            f->eax = create_file(curr_name, initial_size);
            break;
        }
        case SYS_REMOVE: {
            char * curr_name = (char*)(*((int*)f->esp + 1));
            if(curr_name == NULL) {
              ourExit(-1);
            }
            if(curr_name == NULL) {
              ourExit(-1);
            }
            f->eax = remove_file(curr_name);
            f->eax = filesys_remove(curr_name);
            break;
        }
        case SYS_OPEN: {
            char* curr_name = (char*)(*((int*)f->esp + 1));
            if(curr_name == NULL) {
              f->eax = -1;
              return;
            }
            f->eax = open_file(curr_name);
            break;
        }
        case SYS_FILESIZE: {
            break;
        }
        case SYS_READ: {
            break;
        }
        case SYS_WRITE: {
            int fd = *((int*)f->esp + 1);
            void* buffer = (void*)(*((int*)f->esp + 2));
            unsigned size = *((unsigned*)f->esp + 3);
            //run the syscall, a function of your own making
            //since this syscall returns a value, the return value should be stored in f->eax
            f->eax = write(fd, buffer, size);
            break;
        }
        case SYS_SEEK: {
            break;
        }
        case SYS_TELL: {
            break;
        }
        case SYS_CLOSE: {
            break;
        }
        default: {
            thread_exit();
        }
    }
}

static struct thread * get_process_with_specific_tid (tid_t tid){
  struct thread * t;
  for (struct list_elem *iter = list_begin(&all_list);
       iter != list_end(&all_list);
       iter = list_next(iter))
  {
    if(tid == list_entry(iter, struct thread, allelem)->tid) {
      t = list_entry(iter, struct thread, allelem);
      return t;
    }
  }
  return NULL;
}

static tid_t execute(char * cmd_line) {
  tid_t pid = process_execute(cmd_line);
  struct thread * t = get_process_with_specific_tid(pid);
  if(t==NULL) return -1;
  list_push_back(&thread_current() -> children_list,  &t->child_list_elem);
  return pid;
}

static int wait(tid_t pid){
  struct thread * t = get_process_with_specific_tid(pid);
  if(t == NULL || t->parent != thread_current())
    return -1;
}

static int create_file(char * curr_name, off_t initial_size) {
        return filesys_create(curr_name,initial_size);
}

static int remove_file(char * curr_name) {
        return filesys_create(curr_name);
}

static int open_file(char * curr_name) {
    struct file * curr_file = filesys_open(curr_name);
    if(curr_file != NULL) {
        thread_current()->fd++;
        return thread_current()->fd;
    } else {
        return -1;
    }
}

static void ourExit(int status) {
    printf("%s: exit(%d)\n",thread_current()->name,status);
    thread_exit();
}

static uint32_t write(int fd, void *buffer, unsigned int size) {
    if (fd == 1) {
        putbuf(buffer, size);
        return size;
    }
    return 0;
}
