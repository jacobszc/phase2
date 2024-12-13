#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
void* validate_pointer(const void*);
struct chosen_file* list_search(struct list* files, int fd); // search file in the list

extern bool running;

struct chosen_file {
	struct file* ptr;
	int fd;
	struct list_elem elem;
};


///// sys call prototypes, move to syscall.h later
void create_wrap(int *p, struct intr_frame *f);  // this is prolly dumb to pass both pointers. mabey look at later with solution
void exit_wrap(int *p);
void exec_wrap(int *p, struct intr_frame *f);
void sys_wait(int *p, struct intr_frame *f);
void remove_wrap(int *p, struct intr_frame *f);
void open_wrap(int *p, struct intr_frame *f);
void filesize_wrap(int *p, struct intr_frame *f);
void read_wrap(int *p, struct intr_frame *f);
void write_wrap(int *p, struct intr_frame *f);
void seek_wrap(int *p, struct intr_frame *f);
void tell_wrap(int *p, struct intr_frame *f);
void close_wrap(int *p, struct intr_frame *f);

/////////////////////////






void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp; // get the stack pointer

	validate_pointer(p); // check if the pointer is valid



  int SYSCALL_NUM = * p;
	switch (SYSCALL_NUM)
	{
		case SYS_HALT:
		shutdown_power_off();  // dont need a function for this one. jsut call power off right away
		break;

		case SYS_EXIT:
		exit_wrap(p);  // this one does interact with the frame stack directly so jsut p will do as a parameter
		break;

		case SYS_EXEC:
    exec_wrap(p,f);
		
		break;

		case SYS_WAIT:
    wait_wrap(p,f);
		
		break;

		case SYS_CREATE:
		create_wrap(p,f);
		break;

		case SYS_REMOVE:
		remove_wrap(p,f);
		break;

		case SYS_OPEN:
		open_wrap(p,f);
		break;

		case SYS_FILESIZE:
    filesize_wrap(p,f); 
		
		break;

		case SYS_READ:
    read_wrap(p,f);
		
		break;

		case SYS_WRITE:
    write_wrap(p,f);
		
		break;

		case SYS_SEEK:
    seek_wrap(p,f);
		
		break;

		case SYS_TELL:
    tell_wrap(p,f);
		
		break;

		case SYS_CLOSE:
    close_wrap(p,f);
		
		break;


		
	}
}

int exec_proc(char *file_name) // this is a helper function to execute a process
{
	acquire_filesys_lock();
	char * fn_cp = malloc (strlen(file_name)+1);
	  strlcpy(fn_cp, file_name, strlen(file_name)+1);
	  
	  char * save_ptr;
	  fn_cp = strtok_r(fn_cp," ",&save_ptr);

	 struct file* f = filesys_open (fn_cp);

	  if(f==NULL)
	  {
	  	release_filesys_lock();
	  	return -1;
	  }
	  else
	  {
	  	file_close(f);
	  	release_filesys_lock();
	  	return process_execute(file_name);
	  }
}

void exit_proc(int status)
{
	//printf("Exit : %s %d %d\n",thread_current()->name, thread_current()->tid, status);
	struct list_elem *e;

      for (e = list_begin (&thread_current()->parent->child_proc); e != list_end (&thread_current()->parent->child_proc);
           e = list_next (e))
        {
          struct child *f = list_entry (e, struct child, elem);
          if(f->tid == thread_current()->tid)
          {
          	f->used = true;
          	f->exit_error = status;
          }
        }


	thread_current()->exit_error = status;

	if(thread_current()->parent->waitingon == thread_current()->tid)
		sema_up(&thread_current()->parent->child_lock);

	thread_exit();
}

void* validate_pointer(const void *vaddr)
{
	if (!is_user_vaddr(vaddr))
	{
		exit_proc(-1);
		return 0;
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		exit_proc(-1);
		return 0;
	}
	return ptr;
}

struct chosen_file* list_search(struct list* files, int fd)
{

	struct list_elem *e; // list element

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct chosen_file *f = list_entry (e, struct chosen_file, elem);
         
          if(f->fd == fd)
          	
            
            return f;
        }
   return NULL;
}

void close_single_file(struct list* files, int fd)
{

	struct list_elem *e;

	struct chosen_file *f;

      for (e = list_begin (files); e != list_end (files);
          
          
           e = list_next (e))
        {
          f = list_entry (e, struct chosen_file, elem);
         
          if(f->fd == fd)
          {
          	file_close(f->ptr);
          
          	list_remove(e);
          }
        }

    free(f);
}

void close_all_files(struct list* files)
{

	struct list_elem *e;

	while(!list_empty(files))
	{
		e = list_pop_front(files);

		struct chosen_file *f = list_entry (e, struct chosen_file, elem);
          
	      	file_close(f->ptr);
	      	list_remove(e);
	      	free(f);


	}

      
}


//// sys call function defs /////////////////////////////////////// 

void create_wrap(int *p, struct intr_frame *f) {


  validate_pointer(p+5);
		
    validate_pointer(*(p+4));
		  acquire_filesys_lock();
		 f->eax = filesys_create(*(p+4),*(p+5));
		
    
    release_filesys_lock();
};

void exit_wrap(int *p) {

  validate_pointer(p+1);
		exit_proc(*(p+1));
};

void exec_wrap(int *p, struct intr_frame *f) {

  validate_pointer(p+1);
		validate_pointer(*(p+1));
		f->eax = exec_proc(*(p+1));

};

void wait_wrap(int *p, struct intr_frame *f) {

 validate_pointer(p+1);
		f->eax = process_wait(*(p+1));
};

void remove_wrap(int *p, struct intr_frame *f) {

  validate_pointer(p+1);
    validate_pointer(*(p+1));
    acquire_filesys_lock();
    f->eax = filesys_remove(*(p+1));
    release_filesys_lock();
};


void open_wrap(int *p, struct intr_frame *f) {

  validate_pointer(p+1);
		validate_pointer(*(p+1));

		acquire_filesys_lock();
		struct file* fptr = filesys_open (*(p+1));
		release_filesys_lock();
		///////////////////////////////////////
    
    if(fptr==NULL)  // if file pointer is null
			f->eax = -1;
		else
		{
			struct chosen_file *pfile = malloc(sizeof(*pfile)); // allocate memory for the file
			pfile->ptr = fptr;
			pfile->fd = thread_current()->fdTally;
			thread_current()->fdTally++; // increment the file descriptor count
			list_push_back (&thread_current()->files, &pfile->elem);
			f->eax = pfile->fd;

		}
};

void filesize_wrap(int *p, struct intr_frame *f) {

 validate_pointer(p+1);
		acquire_filesys_lock();
		f->eax = file_length (list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
};



void read_wrap(int *p, struct intr_frame *f) {

  validate_pointer(p+7);
		validate_pointer(*(p+6));
		if(*(p+5)==0)
		{
			int i;
			uint8_t* buffer = *(p+6);
			for(i=0;i<*(p+7);i++)
				buffer[i] = input_getc();
			f->eax = *(p+7);
		}
		else
		{
			struct chosen_file* fptr = list_search(&thread_current()->files, *(p+5));
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				acquire_filesys_lock();
				f->eax = file_read (fptr->ptr, *(p+6), *(p+7));
				release_filesys_lock();
			}
		}
};

void write_wrap(int *p, struct intr_frame *f) {

  validate_pointer(p+7);
		validate_pointer(*(p+6));
		if(*(p+5)==1)
		{
			putbuf(*(p+6),*(p+7));
			f->eax = *(p+7);
		}
		else
		{
			struct chosen_file* fptr = list_search(&thread_current()->files, *(p+5)); // get the file pointer
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				acquire_filesys_lock();
				f->eax = file_write (fptr->ptr, *(p+6), *(p+7));
				release_filesys_lock();
			}
		}
};

void seek_wrap(int *p, struct intr_frame *f) {
  validate_pointer(p+5);
		acquire_filesys_lock();
		file_seek(list_search(&thread_current()->files, *(p+4))->ptr,*(p+5));
		release_filesys_lock();

};

void tell_wrap(int *p, struct intr_frame *f) {

  validate_pointer(p+1);
		acquire_filesys_lock();
		f->eax = file_tell(list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
};

void close_wrap(int *p, struct intr_frame *f) {

 validate_pointer(p+1);
		acquire_filesys_lock();
		close_single_file(&thread_current()->files,*(p+1));
		release_filesys_lock();
};