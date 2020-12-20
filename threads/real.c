#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/real.h"
#define F 1<<14

struct real * int_to_real(int n); //convert integer n to fixed point
int real_truncate(struct real * x); //convert fixed point x to integer (rounding towards zero)
int real_round(struct real * x);  //convert fixed point x to integer (rounding to nearest
struct real * add_real_real(struct real * x, struct real * y); // returns real x +  real y
struct real * sub_real_real(struct real * x, struct real * y); // returns real x - real y
struct real * add_real_int(struct real * x, int n); // returns real x +  int n
struct real * sub_real_int(struct real * x, int n); // returns real x - int n
struct real * mul_real_real(struct real * x, struct real * y ); // returns real x * real y
struct real * mul_real_int(struct real * x, int n); // returns real x * int n
struct real * div_real_real(struct real * x, struct real * y); // returns real x /real y
struct real * div_real_int(struct real * x, int n) ;// returns real x / int n

struct real r;

//convert integer n to fixed point
struct real * int_to_real(int n)  {
  r.val = n * F;
  return &r;
}

//convert real x to integer
int real_truncate(struct real * x) {
  return x->val / F;
}


//convert fixed point x to integer (rounding to nearest)
int real_round(struct real * x) {
  r.val = x->val;
  if (r.val < 0) {
    r.val -= F/2;
  }
  else if (r.val > 0) {
    r.val += F/2;
  }
  return real_truncate(&r);
}

// returns real x +  real y
struct real * add_real_real(struct real * x, struct real * y) {
  r.val = x->val + y->val;
  return &r;
}


// returns real x +  int n
struct real * add_real_int(struct real * x, int n){
  r.val = x->val + n * F;
  return &r;
}


// returns real x - int n
struct real * sub_real_int(struct real * x, int n){
  r.val = x->val - n * F;
  return &r;
}


// returns real x * real y
struct real * mul_real_real(struct real * x, struct real * y ){
  r.val = ((int64_t) x->val)* y->val / F;
  return &r;
}


// returns real x * int n
struct real * mul_real_int(struct real * x, int n){
  r.val = x->val * n;
  return &r;
}


 // returns real x /real y
struct real * div_real_real(struct real * x, struct real * y){
  r.val = ((int64_t) x->val)* F / y->val;
  return &r;
}


// returns real x / int n
struct real * div_real_int(struct real * x, int n){
  r.val = x->val / n;
  return &r;
}
