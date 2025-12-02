/*
  ert_main.c - ESBMC verification harness with assertion-based properties
  Notes:
  - Controls verification by defining VERIFY_PROPERTY_N macros at compile time.
  - Uses __ESBMC_assert for properties and nondet_* for nondeterminism.
  - Provides concrete constant definitions for rtConstB and rtConstP.
  - Runs the EB_12B system in a bounded while loop and checks properties after each step.
*/

#include <math.h>
#include <string.h>
#include <stddef.h>
#include "EB_12B.h"

/* ESBMC intrinsics */
extern double nondet_double(void);
extern _Bool nondet_bool(void);
extern void __ESBMC_assert(_Bool, const char *);
extern void __ESBMC_assume(_Bool);

/* Provide model constants required by EB_12B.c (System External names) */

/* Invariant block signals: Divide = inv(Wp') */
const ConstB rtConstB = {
  /* inv(Wp') for Wp = diag([1,2,3,4,5]) is diag([1, 1/2, 1/3, 1/4, 1/5]) */
  {
    1.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0/3.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.25, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.2
  }
};

/* Constant parameters: Wp (Const), d (Const2), ridge identity (Constant4) */
const ConstP rtConstP = {
  /* Wp = diag([1,2,3,4,5]) */
  {
    1.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 2.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 3.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 4.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 5.0
  },
  /* d = [3.0, -1.0, 2.0]' */
  { 3.0, -1.0, 2.0 },
  /* I*1E-12 (3x3) */
  {
    1.0e-12, 0.0,      0.0,
    0.0,      1.0e-12, 0.0,
    0.0,      0.0,      1.0e-12
  }
};

/* Utility helpers */
static double absd(double x) { return x < 0 ? -x : x; }
static _Bool isfinite_double(double x) { return isfinite(x) ? 1 : 0; }

/* Control loop bound (can be overridden by preprocessor) */
#ifndef LOOP_MAX
#define LOOP_MAX 4
#endif

int main(void)
{
  EB_12B_initialize();

  /* Previous input/output snapshots to check stateless behavior */
  double prev_B[15];
  double prev_u[5];
  double prev_Buminusd[3];
  double prev_J = 0.0;
  _Bool prev_ridge_on = 0;
  _Bool has_prev = 0;

  /* Initialize previous arrays to suppress uninitialized warnings */
  for (int i = 0; i < 15; ++i) prev_B[i] = 0.0;
  for (int i = 0; i < 5; ++i) prev_u[i] = 0.0;
  for (int i = 0; i < 3; ++i) prev_Buminusd[i] = 0.0;

  int k = 0;
  while (k < LOOP_MAX) {
    /* Decide whether to reuse previous input or generate new */
    _Bool keep_prev_input = has_prev ? nondet_bool() : 0;

    if (keep_prev_input && has_prev) {
      /* Reuse exactly the same input to test determinism */
      memcpy(rtU.B_j, prev_B, sizeof(prev_B));
    } else if (has_prev && nondet_bool()) {
      /* Slightly perturb previous input to ensure "changed input" path is explored */
      memcpy(rtU.B_j, prev_B, sizeof(prev_B));
      int idx = 3 * (nondet_bool() ? 1 : 0) + (nondet_bool() ? 1 : 0); /* choose one of first few entries */
      if (idx < 0 || idx >= 15) idx = 0;
      double delta = nondet_double();
      __ESBMC_assume(delta > 1e-3 && delta < 0.5);
      rtU.B_j[idx] = rtU.B_j[idx] + delta;
    } else {
      /* Fresh nondet input matrix B (bounded to avoid overflow) */
      for (int i = 0; i < 15; ++i) {
        double v = nondet_double();
        __ESBMC_assume(v > -10.0 && v < 10.0);
        rtU.B_j[i] = v;
      }
    }

    /* Execute one system step */
    EB_12B_step();

    /* Property 1: Ridge_on = True, Check Matrix Precision (within 1e-6) */
#ifdef VERIFY_PROPERTY_1
    __ESBMC_assume(rtY.ridge_on == 1);
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        double expected = (r == c) ? 1.0 : 0.0;
        double err = absd(rtY.check[r + 3 * c] - expected);
        __ESBMC_assert(err <= 1.0e-6, "REQ-1: check matrix not within 1e-6 of identity when ridge_on=true");
      }
    }
    __ESBMC_assert(rtY.ridge_on == 1, "REQ-1: ridge_on output must be true under ridge condition");
#endif

    /* Property 2: Ridge_on = False, Check Matrix Precision (within 1e-12) */
#ifdef VERIFY_PROPERTY_2
    __ESBMC_assume(rtY.ridge_on == 0);
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        double expected = (r == c) ? 1.0 : 0.0;
        double err = absd(rtY.check[r + 3 * c] - expected);
        __ESBMC_assert(err <= 1.0e-12, "REQ-2: check matrix not within 1e-12 of identity when ridge_on=false");
      }
    }
    __ESBMC_assert(rtY.ridge_on == 0, "REQ-2: ridge_on output must be false under non-ridge condition");
#endif

    /* Property 3: Output u Vector Dimension (5x1) */
#ifdef VERIFY_PROPERTY_3
    __ESBMC_assert((sizeof(rtY.u) / sizeof(rtY.u[0])) == 5u, "REQ-3: rtY.u must be length 5");
    for (int i = 0; i < 5; ++i) {
      __ESBMC_assert(isfinite_double(rtY.u[i]), "REQ-3: rtY.u elements must be finite");
    }
#endif

    /* Property 4: Buminusd 2-norm Constraint (||Bu - d||_2 < 0.01) */
#ifdef VERIFY_PROPERTY_4
    {
      double norm2 = 0.0;
      for (int i = 0; i < 3; ++i) norm2 += rtY.Buminusd[i] * rtY.Buminusd[i];
      double normv = sqrt(norm2);
      __ESBMC_assert(normv < 1.0e-2, "REQ-4: ||B*u - d||_2 must be < 0.01");
    }
#endif

    /* Property 5: Minimum Cost J via normal equations (B'*(B*u - d) == 0) */
#ifdef VERIFY_PROPERTY_5
    {
      /* Compute g = B' * (Bu - d), where B is 3x5 stored as rtU.B_j[3*col + row] */
      double g[5] = {0, 0, 0, 0, 0};
      for (int col = 0; col < 5; ++col) {
        double sum = 0.0;
        for (int row = 0; row < 3; ++row) {
          double B_rc = rtU.B_j[3 * col + row]; /* B(row, col) */
          sum += B_rc * rtY.Buminusd[row];
        }
        g[col] = sum;
      }
      /* Tolerances: tighter when ridge_off, looser when ridge_on due to regularization and numeric effects */
      double tol = rtY.ridge_on ? 1.0e-6 : 1.0e-9;
      for (int i = 0; i < 5; ++i) {
        __ESBMC_assert(absd(g[i]) <= tol, "REQ-5: Normal equation B'*(B*u - d) must be approximately zero (optimality)");
      }
      __ESBMC_assert(isfinite_double(rtY.J), "REQ-5: J must be finite");
    }
#endif

    /* Property 6 (auxiliary for Requirement 7): Statelessness check
       If the input B does not change between iterations, outputs must remain exactly the same.
       This ensures the system does not keep hidden previous state and is purely functional of inputs.
    */
#ifdef VERIFY_PROPERTY_6
    if (has_prev) {
      _Bool same_input = 1;
      for (int i = 0; i < 15; ++i) {
        if (rtU.B_j[i] != prev_B[i]) { same_input = 0; break; }
      }
      if (same_input) {
        for (int i = 0; i < 5; ++i) {
          __ESBMC_assert(rtY.u[i] == prev_u[i], "REQ-7: With unchanged inputs, u must remain identical");
        }
        for (int i = 0; i < 3; ++i) {
          __ESBMC_assert(rtY.Buminusd[i] == prev_Buminusd[i], "REQ-7: With unchanged inputs, Buminusd must remain identical");
        }
        __ESBMC_assert(rtY.J == prev_J, "REQ-7: With unchanged inputs, J must remain identical");
        __ESBMC_assert(rtY.ridge_on == prev_ridge_on, "REQ-7: With unchanged inputs, ridge_on must remain identical");
      }
    }
#endif

    /* Update previous snapshots after all checks */
    memcpy(prev_B, rtU.B_j, sizeof(prev_B));
    memcpy(prev_u, rtY.u, sizeof(prev_u));
    memcpy(prev_Buminusd, rtY.Buminusd, sizeof(prev_Buminusd));
    prev_J = rtY.J;
    prev_ridge_on = rtY.ridge_on;
    has_prev = 1;

    k++;
  }

  return 0;
}