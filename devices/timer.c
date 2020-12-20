#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void)
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void)
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1))
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (loops_per_tick | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void)
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then)
{
  return timer_ticks () - then;
}

/* comparator for inserting elemtns in sorted list*/
bool less_time_cmp(const struct list_elem* a, const struct list_elem* b, void* aux UNUSED){
  struct thread* t1 = list_entry(a, struct thread, sleeping_elem);
  struct thread* t2 = list_entry(b, struct thread, sleeping_elem);
  if(t1->time_to_wake_up_snow_white == t2->time_to_wake_up_snow_white)
  {
    return t1->priority > t2->priority;
  }
  return t1->time_to_wake_up_snow_white < t2->time_to_wake_up_snow_white;
}


/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks)
{
  enum intr_level old_level;
  int64_t start = timer_ticks ();
  ASSERT (intr_get_level () == INTR_ON);
  thread_current()-> time_to_wake_up_snow_white = ticks + timer_ticks();
  list_insert_ordered(&sleeping_threads,&thread_current()->sleeping_elem, &less_time_cmp, NULL);
  old_level = intr_disable();
  thread_block();
  intr_set_level (old_level);
}


/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void
timer_msleep (int64_t ms)
{
  real_time_sleep (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void
timer_usleep (int64_t us)
{
  real_time_sleep (us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void
timer_nsleep (int64_t ns)
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void
timer_mdelay (int64_t ms)
{
  real_time_delay (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void
timer_udelay (int64_t us)
{
  real_time_delay (us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void
timer_ndelay (int64_t ns)
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void)
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}


/*wakes up snow white*/
static void
wake_up_snow_white(){
  struct thread * t;
  while(!list_empty(&sleeping_threads) &&  (t = list_entry(list_front(&sleeping_threads), struct thread, sleeping_elem))->time_to_wake_up_snow_white <= timer_ticks() && t != NULL){
    list_pop_front(&sleeping_threads);
    thread_unblock(t);
  }
}

static void calculate_recent_cpu_for_all_threads() {
  if(thread_current()->status == THREAD_RUNNING)
  thread_current()->recent_cpu.val = add_real_int(&thread_current()->recent_cpu,1).val;
  for(struct list_elem* iter = list_begin(&all_list);
      iter != list_end(&all_list);
      iter = list_next(iter))
  {
    struct thread * t = list_entry(iter, struct thread, allelem);
    struct real x,y,z;
    x = mul_real_int(&load_avg, 2);
    y = mul_real_int(&load_avg, 2);
    y = add_real_int(&y, 1);
    z = div_real_real(&x,&y);
    x = mul_real_real(&z, &t->recent_cpu);
    y = add_real_int(&x, t->nice);
    t->recent_cpu.val = y.val;
  }
}

static void
update_load_average() {
  struct real x,y,z;
  x = int_to_real(59);
  if(DEBUG)printf("1 is %d\n", x.val);
  y = int_to_real(60);
  if(DEBUG)printf("2 is %d\n", y.val);
  z = div_real_real(&x,&y);
  if(DEBUG)printf("3 is %d\n",z.val);
  z = mul_real_real(&z,&load_avg);
  if(DEBUG)printf("4 is %d\n", z.val);
  x = int_to_real(1);
  if(DEBUG)printf("5 is %d\n", x.val);
  x = div_real_real(&x,&y);
  if(DEBUG)printf("6 is %d\n", x.val);
  int ready_size = (int) list_size(&ready_list);
  if(thread_current() != idle_thread) ready_size++;
  x = mul_real_int(&x, ready_size);
  if(DEBUG)printf("7 is %d\n", x.val);
  load_avg = add_real_real(&z,&x);
  if(DEBUG)printf("load_avg is %d\n\n\n", load_avg.val);
  struct real xz = mul_real_int(&load_avg,100);
  if(DEBUG)printf("load_avg_multiplication from func is %d\n\n\n", xz.val);
  if(DEBUG)printf("load_avg from func is %d\n\n\n", real_truncate(&xz));
}

void
update_priority_of_all_threads() {
  for(struct list_elem* iter = list_begin(&all_list);
      iter != list_end(&all_list);
      iter = list_next(iter))
  {
    struct thread * t = list_entry(iter, struct thread, allelem);
    mlfqs_set_priority_for_specific_thread(t);
  }
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  wake_up_snow_white();
  if(timer_ticks() % TIMER_FREQ == 0 && thread_mlfqs) {
    update_load_average();
    calculate_recent_cpu_for_all_threads();
  }
  if(timer_ticks() % 4 == 0 && thread_mlfqs) {
    update_priority_of_all_threads();
    list_sort(&ready_list, &more_priority_cmp, NULL);
  }
  thread_tick ();
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops)
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait (loops);

  /* If the tick count changed, we iterated too long. */
  barrier ();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops)
{
  while (loops-- > 0)
    barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom)
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.

        (NUM / DENOM) s
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         timer_sleep() because it will yield the CPU to other
         processes. */
      timer_sleep (ticks);
    }
  else
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      real_time_delay (num, denom);
    }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
}
