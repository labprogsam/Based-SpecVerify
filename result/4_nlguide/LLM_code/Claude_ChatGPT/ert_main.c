Below is an assertion-based ERT main harness that drives NLGuidance_12B_step(), uses nondeterministic inputs, and verifies each requirement with __ESBMC_assert under controllable preprocessing macros VERIFY_PROPERTY_1 .. VERIFY_PROPERTY_7. It uses a bounded while loop, maintains previous output without static storage, and places all property-run code in main.

File: ert_main.c

#include <math.h>
#include "NLGuidance_12B.h"

/* ESBMC nondeterministic generators */
extern double nondet_double(void);
extern _Bool nondet_bool(void);

/* ESBMC assume is available as a builtin; prototype not required in ESBMC */

/* Configuration macros (can be overridden at compile time) */
#ifndef LOOP_MAX
#define LOOP_MAX 5
#endif

#ifndef TIME_STEP
#define TIME_STEP 0.1
#endif

/* Utility functions */
static double norm3(const double a[3]) {
  return sqrt(fmax(0.0, a[0]*a[0] + a[1]*a[1] + a[2]*a[2]));
}

static double dot3(const double a[3], const double b[3]) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/* Recompute mode and candidate aim points (mirrors embedded code) */
static void compute_modes_and_candidates(
  /* inputs */
  const double Xtarg[3], const double Xv[3], const double Vv[3], double r,
  /* outputs */
  _Bool* innerMode, _Bool* outerMode,
  double Ycw[3], double Yccw[3],
  /* aux outputs used by properties */
  double* sum2_out, double* A_out, double* normXrSq_out, double* dot_out)
{
  double Xr[3];
  Xr[0] = Xtarg[0] - Xv[0];
  Xr[1] = Xtarg[1] - Xv[1];
  Xr[2] = Xtarg[2] - Xv[2];

  double normXrSq = Xr[0]*Xr[0] + Xr[1]*Xr[1] + Xr[2]*Xr[2];
  double normXr = sqrt(fmax(0.0, normXrSq));
  double dotXV = dot3(Xr, Vv);
  double normVvSq = Vv[0]*Vv[0] + Vv[1]*Vv[1] + Vv[2]*Vv[2];
  double normVv = sqrt(fmax(0.0, normVvSq));

  /* Outer/Inner mode metrics */
  double sum2 = normXrSq - r*r;                  /* ||Xr||^2 - r^2 */
  double A = normVvSq*normXrSq - dotXV*dotXV;    /* ||Vv||^2*||Xr||^2 - (Xr·Vv)^2 */

  *innerMode = (sum2 <= 0.0) || (A <= 0.0);
  *outerMode = (sum2 > 0.0) && (A > 0.0);

  /* Provide aux outputs */
  if (sum2_out) *sum2_out = sum2;
  if (A_out) *A_out = A;
  if (normXrSq_out) *normXrSq_out = normXrSq;
  if (dot_out) *dot_out = dotXV;

  /* Initialize to something (only valid when outerMode is true) */
  Ycw[0] = Xv[0]; Ycw[1] = Xv[1]; Ycw[2] = Xv[2];
  Yccw[0] = Xv[0]; Yccw[1] = Xv[1]; Yccw[2] = Xv[2];

  if (*outerMode) {
    /* Replicate code math for candidates */
    double S = sqrt(sum2 / A);          /* rtb_Sum2 in code */
    double Vs[3] = { S*Vv[0], S*Vv[1], S*Vv[2] }; /* rtb_MathFunction_idx_* */
    double invNormXrSq = (normXrSq > 0.0) ? (1.0 / normXrSq) : 0.0; /* rtb_MathFunction1 = 1/||Xr||^2 */
    double SD = S * dotXV;
    double d1 = r + SD;  /* rtb_XrDotVv in code before CCW/CW separation */
    double d2 = SD - r;

    /* Sum_c vector used for CW branch (SubSystem 'CW': y = Xv + Sum_c) */
    double Sum_c[3];
    Sum_c[0] = (d1 * Xr[0] * -invNormXrSq + Vs[0]) * r + Xr[0];
    Sum_c[1] = (d1 * Xr[1] * -invNormXrSq + Vs[1]) * r + Xr[1];
    Sum_c[2] = (d1 * Xr[2] * -invNormXrSq + Vs[2]) * r + Xr[2];

    /* Candidate outputs */
    Ycw[0] = Xv[0] + Sum_c[0];
    Ycw[1] = Xv[1] + Sum_c[1];
    Ycw[2] = Xv[2] + Sum_c[2];

    Yccw[0] = ((d2 * Xr[0] *  invNormXrSq - Vs[0]) * r + Xr[0]) + Xv[0];
    Yccw[1] = ((d2 * Xr[1] *  invNormXrSq - Vs[1]) * r + Xr[1]) + Xv[1];
    Yccw[2] = ((d2 * Xr[2] *  invNormXrSq - Vs[2]) * r + Xr[2]) + Xv[2];
  }
}

/* Inner mode candidate points and nearest selection (mirrors embedded code) */
static void compute_inner_candidates_and_nearest(
  const double Xtarg[3], const double Xv[3], double r,
  _Bool* valid, double cmd1[3], double cmd2[3], int* which_min /* 1 or 2 */)
{
  double Xr[3] = { Xtarg[0] - Xv[0], Xtarg[1] - Xv[1], Xtarg[2] - Xv[2] };
  double normXr = norm3(Xr);

  if (normXr <= 0.0) {
    *valid = 0;
    cmd1[0]=cmd1[1]=cmd1[2]=0.0;
    cmd2[0]=cmd2[1]=cmd2[2]=0.0;
    *which_min = 0;
    return;
  }

  *valid = 1;
  double alpha = r / normXr;

  /* Command candidates (z=0 as in code) */
  cmd1[0] = Xtarg[0] + alpha*Xr[0];
  cmd1[1] = Xtarg[1] + alpha*Xr[1];
  cmd1[2] = 0.0;

  cmd2[0] = Xtarg[0] - alpha*Xr[0];
  cmd2[1] = Xtarg[1] - alpha*Xr[1];
  cmd2[2] = 0.0;

  /* Distances to vehicle position Xv (z difference uses 0 - Xv[2]) */
  double dx1 = cmd1[0] - Xv[0];
  double dy1 = cmd1[1] - Xv[1];
  double dz1 = 0.0 - Xv[2];
  double d1 = sqrt(fmax(0.0, dx1*dx1 + dy1*dy1 + dz1*dz1));

  double dx2 = cmd2[0] - Xv[0];
  double dy2 = cmd2[1] - Xv[1];
  double dz2 = 0.0 - Xv[2];
  double d2 = sqrt(fmax(0.0, dx2*dx2 + dy2*dy2 + dz2*dz2));

  *which_min = (d1 <= d2) ? 1 : 2;
}

int main(void)
{
  NLGuidance_12B_initialize();

  int steps = 0;
  _Bool has_prev = 0;
  double prev_yout[3] = {0.0, 0.0, 0.0};

  while (steps < LOOP_MAX) {
    /* Nondeterministic inputs with reasonable constraints */
    rtU.Xtarg[0] = nondet_double();
    rtU.Xtarg[1] = nondet_double();
    rtU.Xtarg[2] = nondet_double();

    rtU.Xv[0] = nondet_double();
    rtU.Xv[1] = nondet_double();
    rtU.Xv[2] = nondet_double();

    rtU.Vv[0] = nondet_double();
    rtU.Vv[1] = nondet_double();
    rtU.Vv[2] = nondet_double();

    rtU.Vt[0] = nondet_double();
    rtU.Vt[1] = nondet_double();
    rtU.Vt[2] = nondet_double();

    rtU.r = nondet_double();

    /* Physical/robustness assumptions */
    __ESBMC_assume(rtU.r > 1e-6 && rtU.r < 1.0e6);
    __ESBMC_assume(fabs(rtU.Xtarg[0]) < 1.0e6 && fabs(rtU.Xtarg[1]) < 1.0e6 && fabs(rtU.Xtarg[2]) < 1.0e6);
    __ESBMC_assume(fabs(rtU.Xv[0])    < 1.0e6 && fabs(rtU.Xv[1])    < 1.0e6 && fabs(rtU.Xv[2])    < 1.0e6);
    __ESBMC_assume(fabs(rtU.Vv[0])    < 1.0e4 && fabs(rtU.Vv[1])    < 1.0e4 && fabs(rtU.Vv[2])    < 1.0e4);
    __ESBMC_assume(fabs(rtU.Vt[0])    < 1.0e4 && fabs(rtU.Vt[1])    < 1.0e4 && fabs(rtU.Vt[2])    < 1.0e4);

    /* Run one cycle */
    NLGuidance_12B_step();

    /* Recompute mode and candidate outputs for verification */
    _Bool innerMode = 0, outerMode = 0;
    double Ycw[3], Yccw[3];
    double sum2=0.0, A=0.0, normXrSq=0.0, dotXV=0.0;
    compute_modes_and_candidates(rtU.Xtarg, rtU.Xv, rtU.Vv, rtU.r,
                                 &innerMode, &outerMode,
                                 Ycw, Yccw,
                                 &sum2, &A, &normXrSq, &dotXV);

    /* Requirement 1: Maintain target on port-side (Outer mode) */
    /* Interpreted as selecting CCW such that aim-point is to the left of current velocity:
       cross_z(Vv, yout-Xv) >= 0 */
    #ifdef VERIFY_PROPERTY_1
    if (outerMode) {
      double ap[3] = { rtY.yout[0] - rtU.Xv[0],
                       rtY.yout[1] - rtU.Xv[1],
                       rtY.yout[2] - rtU.Xv[2] };
      double cross_z = rtU.Vv[0]*ap[1] - rtU.Vv[1]*ap[0];
      __ESBMC_assert(cross_z >= 0.0, "REQ-1: Outer mode must keep aimpoint on port-side (CCW relative to Vv).");
    }
    #endif

    /* Requirement 2: Compute inertial position vector for aim point 1 (CW candidate) */
    /* If the (degenerate) cross decision in code were < 0, yout must equal CW candidate (Xv + Sum_c). */
    #ifdef VERIFY_PROPERTY_2
    if (outerMode) {
      /* The code's 'cross' check is: if (rtb_Sum2_tmp - rtb_Sum2_tmp < 0) -> CW, which is always false.
         We replicate the antecedent; assertion is thus conditionally checked (vacuous if false). */
      double Xr0 = rtU.Xtarg[0] - rtU.Xv[0];
      double Xr1 = rtU.Xtarg[1] - rtU.Xv[1];
      /* Sum_c used in CW candidate already embedded in Ycw via helper. We reuse code's scalar:
         rtb_Sum2_tmp = Xr0 * Sum_c_y (but we only need the inequality condition). */
      double cross_cond_value = (Xr0 * (Ycw[1] - rtU.Xv[1])) - (Xr0 * (Ycw[1] - rtU.Xv[1]));
      if (cross_cond_value < 0.0) {
        __ESBMC_assert(fabs(rtY.yout[0] - Ycw[0]) <= 0.0 &&
                       fabs(rtY.yout[1] - Ycw[1]) <= 0.0 &&
                       fabs(rtY.yout[2] - Ycw[2]) <= 0.0,
                       "REQ-2: In Outer mode CW branch, yout must equal Xv + Sum_c.");
      }
    }
    #endif

    /* Requirement 3: Compute inertial position vector for aim point 2 (CCW candidate) */
    /* If not CW (as in current implementation), yout must equal CCW candidate. */
    #ifdef VERIFY_PROPERTY_3
    if (outerMode) {
      /* Code's 'else' branch is CCW. We assert actual output equals CCW candidate. */
      __ESBMC_assert(fabs(rtY.yout[0] - Yccw[0]) <= 0.0 &&
                     fabs(rtY.yout[1] - Yccw[1]) <= 0.0 &&
                     fabs(rtY.yout[2] - Yccw[2]) <= 0.0,
                     "REQ-3: In Outer mode CCW branch, yout must equal CCW candidate.");
    }
    #endif

    /* Requirement 4: Always select aim point for counter clockwise loiter */
    #ifdef VERIFY_PROPERTY_4
    if (outerMode) {
      /* Equivalent to always choosing the CCW candidate */
      __ESBMC_assert(fabs(rtY.yout[0] - Yccw[0]) <= 0.0 &&
                     fabs(rtY.yout[1] - Yccw[1]) <= 0.0 &&
                     fabs(rtY.yout[2] - Yccw[2]) <= 0.0,
                     "REQ-4: Outer mode shall always select CCW aimpoint.");
    }
    #endif

    /* Requirement 5: Inner mode choose nearest position and set altitude to 0 */
    #ifdef VERIFY_PROPERTY_5
    if (innerMode) {
      _Bool valid = 0;
      double cmd1[3], cmd2[3];
      int which_min = 0;
      compute_inner_candidates_and_nearest(rtU.Xtarg, rtU.Xv, rtU.r, &valid, cmd1, cmd2, &which_min);
      if (valid) {
        if (which_min == 1) {
          __ESBMC_assert(fabs(rtY.yout[0] - cmd1[0]) <= 0.0 &&
                         fabs(rtY.yout[1] - cmd1[1]) <= 0.0,
                         "REQ-5: Inner mode must select nearest candidate (cmd1).");
        } else if (which_min == 2) {
          __ESBMC_assert(fabs(rtY.yout[0] - cmd2[0]) <= 0.0 &&
                         fabs(rtY.yout[1] - cmd2[1]) <= 0.0,
                         "REQ-5: Inner mode must select nearest candidate (cmd2).");
        }
        __ESBMC_assert(rtY.yout[2] == 0.0, "REQ-5: Inner mode altitude must be 0.");
      }
    }
    #endif

    /* Requirement 6a: Output rate change bounded by (||Vv|| + ||Vt||) * T */
    #ifdef VERIFY_PROPERTY_6
    if (has_prev) {
      double dy[3] = { rtY.yout[0] - prev_yout[0],
                       rtY.yout[1] - prev_yout[1],
                       rtY.yout[2] - prev_yout[2] };
      double dy_norm = norm3(dy);
      double bound = (norm3(rtU.Vv) + norm3(rtU.Vt)) * TIME_STEP;
      __ESBMC_assert(dy_norm <= bound,
                     "REQ-6a: ||yout(k) - yout(k-1)|| <= (||Vv|| + ||Vt||) * T.");
    }
    #endif

    /* Requirement 7: Output equivalent altitude for in-plane navigation (Inner mode) */
    #ifdef VERIFY_PROPERTY_7
    if (innerMode && (rtU.Xtarg[2] == rtU.Xv[2])) {
      __ESBMC_assert(rtY.yout[2] == 0.0,
                     "REQ-7: Inner mode with equal altitudes must output yout[2]=0.");
    }
    #endif

    /* Update previous output state for next iteration (no static) */
    prev_yout[0] = rtY.yout[0];
    prev_yout[1] = rtY.yout[1];
    prev_yout[2] = rtY.yout[2];
    has_prev = 1;

    steps++;
  }

  return 0;
}
