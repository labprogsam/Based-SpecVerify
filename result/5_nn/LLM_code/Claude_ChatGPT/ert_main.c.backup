Here is an ESBMC-ready ert_main.c harness that exercises the provided model, runs it in a bounded while loop, and verifies all requirements using __ESBMC_assert under VERIFY_PROPERTY_N preprocessing macros. It uses nondeterministic inputs and maintains previous-state variables without static storage.

Code: ert_main.c
#include <math.h>
#include <stdint.h>
#include "nn_12B.h"

/* ESBMC intrinsics */
extern double nondet_double(void);
extern int nondet_int(void);
extern _Bool nondet_bool(void);
extern void __ESBMC_assert(_Bool cond, const char* msg);

/* Allow external control of steps via macro; default to a reasonable value */
#ifndef MAX_STEPS
#define MAX_STEPS 6u
#endif

int main(void)
{
  /* Initialize model */
  nn_12B_initialize();

  /* Previous-state tracking (no static) */
  double prev_x = 0.0;
  double prev_y = 0.0;
  double prev_z = 0.0;
  _Bool have_prev = 0;

  /* Optional dataset placeholders for Requirement 4 (truth data) */
  double xt = 0.0;
  double yt = 0.0;
  double zt = 0.0;

  /* Bounded iterative execution */
  unsigned step = 0u;
  while (step < MAX_STEPS) {
    /* Drive inputs nondeterministically each iteration */
    rtU.x = nondet_double();
    rtU.y = nondet_double();

    /* Run one cycle (pure feedforward; z is available after step) */
    nn_12B_step();

    /* Requirement 1: Maximum output z <= 1.1 */
    #ifdef VERIFY_PROPERTY_1
    __ESBMC_assert(rtY.z <= 1.1, "R1: Max output z must be <= 1.1");
    #endif

    /* Requirement 2: Minimum output z >= -0.2 */
    #ifdef VERIFY_PROPERTY_2
    __ESBMC_assert(rtY.z >= -0.2, "R2: Min output z must be >= -0.2");
    #endif

    /* Requirement 3: Spatial derivatives bounds -35 <= Δz/Δx <= 10 and -35 <= Δz/Δy <= 10
       Needs at least two iterations to compare with previous state. */
    #ifdef VERIFY_PROPERTY_3
    if (have_prev) {
      const double dx = rtU.x - prev_x;
      const double dy = rtU.y - prev_y;
      const double dz = rtY.z - prev_z;

      if (dx != 0.0) {
        const double dzdx = dz / dx;
        __ESBMC_assert(dzdx >= -35.0 && dzdx <= 10.0,
                       "R3: Spatial derivative wrt x out of bounds");
      }
      if (dy != 0.0) {
        const double dzdy = dz / dy;
        __ESBMC_assert(dzdy >= -35.0 && dzdy <= 10.0,
                       "R3: Spatial derivative wrt y out of bounds");
      }
    }
    #endif

    /* Requirement 4: Absolute error |z - zt| <= 0.01 for truth sample (xt, yt, zt).
       This harness expects an external environment to supply zt consistently with (xt, yt).
       Here we bind xt, yt to the current inputs to enable per-iteration checking. */
    #ifdef VERIFY_PROPERTY_4
    xt = rtU.x;
    yt = rtU.y;
    zt = nondet_double(); /* Expected truth must be provided by environment/dataset */
    {
      const double err = fabs(rtY.z - zt);
      __ESBMC_assert(err <= 0.01, "R4: Absolute error to truth exceeds 0.01");
    }
    #endif

    /* Additional check per instruction: if inputs keep previous values, output must keep previous value */
    #ifdef VERIFY_PROPERTY_5
    if (have_prev && (rtU.x == prev_x) && (rtU.y == prev_y)) {
      __ESBMC_assert(rtY.z == prev_z,
                     "R5: Output z must remain unchanged if inputs x and y are unchanged");
    }
    #endif

    /* Update previous-state variables for next iteration */
    prev_x = rtU.x;
    prev_y = rtU.y;
    prev_z = rtY.z;
    have_prev = 1;

    /* Advance loop */
    step++;
  }

  return 0;
}
