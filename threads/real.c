#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/real.h"

struct real int_to_real(int n); //convert integer n to fixed point
int real_truncate(struct real * x); //convert fixed point x to integer (rounding towards zero)
int real_round(struct real * x);  //convert fixed point x to integer (rounding to nearest
struct real add_real_real(struct real * x, struct real * y); // returns real x +  real y
struct real sub_real_real(struct real * x, struct real * y); // returns real x - real y
struct real add_real_int(struct real * x, int n); // returns real x +  int n
struct real sub_real_int(struct real * x, int n); // returns real x - int n
struct real mul_real_real(struct real * x, struct real * y ); // returns real x * real y
struct real mul_real_int(struct real * x, int n); // returns real x * int n
struct real div_real_real(struct real * x, struct real * y); // returns real x /real y
struct real div_real_int(struct real * x, int n) ;// returns real x / int n



//convert integer n to fixed point
struct real int_to_real(int n)  {
  struct real r;
  r.val = n * (1<<14);
  return r;
}

//convert real x to integer
int real_truncate(struct real * x) {
  return (x->val / (1<<14));
}


//convert fixed point x to integer (rounding to nearest)
int real_round(struct real * x) {
  struct real r;
  r.val = x->val;
  if (r.val < 0) {
    r.val -= 1<<13;
  }
  else if (r.val > 0) {
    r.val += 1<<13;
  }
  return real_truncate(&r);
}

// returns real x +  real y
struct real add_real_real(struct real * x, struct real * y) {
  struct real r;
  r.val = x->val + y->val;
  return r;
}


// returns real x +  int n
struct real add_real_int(struct real * x, int n){
  struct real r;
  r.val = x->val + n * (1<<14);
  return r;
}


// returns real x - int n
struct real sub_real_int(struct real * x, int n){
  struct real r;
  r.val = x->val - n * (1<<14);
  return r;
}


// returns real x * real y
struct real mul_real_real(struct real * x, struct real * y ){
  struct real r;
  r.val = ((int64_t) x->val)* y->val / (1<<14);
  return r;
}


// returns real x * int n
struct real mul_real_int(struct real * x, int n){
  struct real r;
  r.val = x->val * n;
  return r;
}


 // returns real x /real y
struct real div_real_real(struct real * x, struct real * y){
  struct real r;
  r.val = ((int64_t) x->val)* (1<<14) / y->val;
  return r;
}


// returns real x / int n
struct real div_real_int(struct real * x, int n){
  struct real r;
  r.val = x->val / n;
  return r;
}
