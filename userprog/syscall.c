#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"

static void syscall_handler(struct intr_frame *);

void
syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED) {
    if (pagedir_get_page(f->esp, 0x20101234) == NULL) {
        thread_exit();
        //ToDo SYS_EXIT
        // يحيى و شعراوي بيضربوا بعض
    }
    switch (*(int *) f->esp) {
        case SYS_HALT: {
            break;
        }
        case SYS_EXIT: {
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