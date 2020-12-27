#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"

static void syscall_handler(struct intr_frame *);

static uint32_t write(int fd, void *pVoid, unsigned int size);

static void ourExit(int status);

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
            break;
        }
        case SYS_EXIT: {
            int status = *((int*)f->esp + 1);
            ourExit(status);
            break;
        }
        case SYS_EXEC: {
            break;
        }
        case SYS_WAIT: {
            break;
        }
        case SYS_CREATE: {
            break;
        }
        case SYS_REMOVE: {
            break;
        }
        case SYS_OPEN: {
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
