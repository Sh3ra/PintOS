#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#define F 1<<14
//convert integer n to fixed point
struct
real int_to_real(int n);  {
  struct real r;
  r->val = n * F;
  return r;
}

//convert real x to integer
int
real_truncate(struct real x) {
  return x->val / F;
}


//convert fixed point x to integer (rounding to nearest)
int
real_round(struct real x) {
  struct real r;
  r->val = x->val;
  if (r->val < 0) {
    r->val -= F/2;
  }
  else if (r->val > 0) {
    r->val += F/2;
  }
  return real_truncate(r);
}

// returns real x +  real y
struct real
add_real_real(struct real x, struct real y) {
  struct real r;
  r->val = x->val + y->val;
  return r;
}


// returns real x - real y
struct real
sub_real_real(struct real x, struct real y){
  struct real r;
  r->val = x->val - y->val;
  return r;
}


// returns real x +  int n
struct real
add_real_int(struct real x, int n){
  struct real r;
  r->val = x->val + n * F;
  return r;
}


// returns real x - int n
struct real
sub_real_int(struct real x, int n){
  struct real r;
  r->val = x->val - n * F;
  return r;
}


// returns real x * real y
struct real
mul_real_real(struct real x, struct real y ){
  struct real r;
  r->val = ((int64_t) x)* y / F;
  return r;
}


// returns real x * int n
struct real
mul_real_int(struct real x, int n){
  struct real r;
  r->val = x->val * n;
  return r;
}


 // returns real x /real y
struct real
div_real_real(struct real x, struct real y){
  struct real r;
  r->val = ((int64_t) x)* F / y;
  return r;
}


// returns real x / int n
struct real
div_real_int(struct real x, int n){
  struct real r;
  r->val = x->val / n;
  return r;
}
