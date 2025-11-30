```c
/*
 * ert_main.c - ESBMC verification harness with assertion-based checks
 * Requirements covered: RM-001, RM-002, RM-003, RM-004
 *
 * Control which property to verify by defining one of:
 *   -DVERIFY_PROPERTY_1   (RM-001 No-fail state failure detection)
 *   -DVERIFY_PROPERTY_2   (RM-002 No-fail state mid-value selection)
 *   -DVERIFY_PROPERTY_3   (RM-003 Single-fail state good channel average)
 *   -DVERIFY_PROPERTY_4   (RM-004 Second failure in progress value retention)
 *
 * You may also override bounds:
 *   -DLOOP_MAX=...
 */

#include "triplex_12B.h"
#include "rtwtypes.h"

/* Non-deterministic generators (ESBMC recognizes these by name/signature) */
double nondet_double(void);
int nondet_int(void);
_Bool nondet_bool(void);

/* Optional ESBMC helpers */
void __ESBMC_assume(_Bool cond);

#ifndef LOOP_MAX
#define LOOP_MAX 8
#endif

/* Helper to compute the median (mid-value) of three doubles (returns a, b, or c exactly) */
static double median3(double a, double b, double c)
{
  double m;
  if ((a >= b && a <= c) || (a <= b && a >= c)) {
    m = a;
  } else if ((b >= a && b <= c) || (b <= a && b >= c)) {
    m = b;
  } else {
    m = c;
  }
  return m;
}

/* Map failing branch to expected FC:
 *  4 => Branch A failed, 2 => Branch B failed, 1 => Branch C failed
 */
static int expected_fc_for_failcase(int fail_case_code)
{
  if (fail_case_code == 4) return 4; /* A */
  if (fail_case_code == 2) return 2; /* B */
  return 1;                           /* default to C */
}

/* Configure inputs for a single-fail miscompare condition that persists:
 *  - Tlevel > 0, d > Tlevel
 *  - Exactly one branch deviates by d and the two others agree
 * fail_case_code: 4 (A fails), 2 (B fails), 1 (C fails)
 * base: nominal base value, d: deviation (d > Tlevel)
 */
static void set_single_fail_inputs(double base, double d, int fail_case_code)
{
  if (fail_case_code == 4) {          /* A fails */
    rtU.ia = base + d;
    rtU.ib = base;
    rtU.ic = base;
  } else if (fail_case_code == 2) {   /* B fails */
    rtU.ia = base;
    rtU.ib = base + d;
    rtU.ic = base;
  } else {                            /* C fails */
    rtU.ia = base;
    rtU.ib = base;
    rtU.ic = base + d;
  }
}

/* Configure inputs for "second failure in progress":
 *  - We are already in single-fail state with FC in {1,2,4}
 *  - Now force the two remaining "good" channels to diverge by > Tlevel
 * The failed branch value can be anything; we vary the two good ones.
 */
static void set_second_fail_in_progress(double base, double d)
{
  if (rtY.FC == 4) {        /* A failed; vary B and C to miscompare */
    rtU.ia = base;          /* A (failed) arbitrary */
    rtU.ib = base + d;
    rtU.ic = base;
  } else if (rtY.FC == 2) { /* B failed; vary A and C */
    rtU.ia = base + d;
    rtU.ib = base;          /* B (failed) arbitrary */
    rtU.ic = base;
  } else {                  /* C failed; vary A and B */
    rtU.ia = base + d;
    rtU.ib = base;
    rtU.ic = base;          /* C (failed) arbitrary */
  }
}

int main(void)
{
  /* Initialize model (globals are zero-initialized; call initialize for completeness) */
  triplex_12B_initialize();

  /* Common input bounds (can be overridden per property) */
  rtU.Tlevel = nondet_double();
  __ESBMC_assume(rtU.Tlevel > 0.0 && rtU.Tlevel <= 1000.0);

  rtU.PClimit = nondet_int();
  __ESBMC_assume(rtU.PClimit >= 1 && rtU.PClimit <= 5);

  /* Reset state for each property block as needed */
#ifdef VERIFY_PROPERTY_1
  {
    /* RM-001: No-fail state failure detection
     * Scenario: start in FC=0, PC=0. Apply a persistent single-fail miscompare.
     * Verify:
     *  - While miscompare persists and prev PC <= PClimit: PC increments by 1, TC increments by 1, FC unchanged.
     *  - When prev PC > PClimit: FC latches to {1,2,4}, PC resets to 0, TC unchanged that cycle.
     */
    /* Reset state to known starting values */
    rtDW.Delay1_DSTATE[0] = 0; /* PC */
    rtDW.Delay1_DSTATE[1] = 0; /* TC */
    rtDW.Delay1_DSTATE[2] = 0; /* FC */

    /* Choose a failing branch and stable inputs */
    int fail_case_code = nondet_int();
    __ESBMC_assume(fail_case_code == 4 || fail_case_code == 2 || fail_case_code == 1);
    int expected_fc = expected_fc_for_failcase(fail_case_code);

    double base = nondet_double();
    __ESBMC_assume(base >= -1.0e6 && base <= 1.0e6);

    double d = nondet_double();
    __ESBMC_assume(d > rtU.Tlevel && d <= rtU.Tlevel + 1000.0);

    set_single_fail_inputs(base, d, fail_case_code);

    /* Number of cycles: must cover until latch (prev PC > PClimit) happens */
    int max_steps = rtU.PClimit + 3;
    if (max_steps < 4) max_steps = 4;

    int step = 0;
    int prevPC = rtDW.Delay1_DSTATE[0];
    int prevTC = rtDW.Delay1_DSTATE[1];
    int prevFC = rtDW.Delay1_DSTATE[2];

    while (step < max_steps) {
      triplex_12B_step();

      /* While still in no-fail and miscompare persists: PC increments, TC increments */
      if (prevFC == 0 && prevPC <= rtU.PClimit) {
        __ESBMC_assert(rtY.PC == prevPC + 1, "RM-001: PC must increment by 1 while miscompare persists and before latch");
        __ESBMC_assert(rtY.TC == prevTC + 1, "RM-001: TC must increment by 1 while miscompare persists");
        __ESBMC_assert(rtY.FC == 0, "RM-001: FC must remain 0 before persistence limit exceeded");
      }

      /* When prev PC > PClimit, latch must occur with expected FC and PC reset */
      if (prevFC == 0 && prevPC > rtU.PClimit) {
        __ESBMC_assert(rtY.FC == expected_fc, "RM-001: FC must latch to expected code when PC > PClimit");
        __ESBMC_assert(rtY.PC == 0, "RM-001: PC must reset to 0 when latching fault code");
        __ESBMC_assert(rtY.TC == prevTC, "RM-001: TC must not change on the latch cycle");
      }

      /* Prepare for next cycle */
      prevPC = rtY.PC;
      prevTC = rtY.TC;
      prevFC = rtY.FC;
      /* Keep the miscompare persistent (inputs unchanged) */
      step++;
    }
  }
#endif /* VERIFY_PROPERTY_1 */

#ifdef VERIFY_PROPERTY_2
  {
    /* RM-002: No-fail state mid-value selection
     * Verify: whenever FC == 0, sel_val equals the median of {ia, ib, ic} in that same cycle.
     * Approach: keep the system out of failure by ensuring no miscompare (Tlevel large) and arbitrary inputs.
     */
    /* Reset state */
    rtDW.Delay1_DSTATE[0] = 0;
    rtDW.Delay1_DSTATE[1] = 0;
    rtDW.Delay1_DSTATE[2] = 0;

    /* Strongly avoid miscompare by using a large Tlevel and bounded inputs */
    rtU.Tlevel = 1.0e9;
    rtU.PClimit = 3;

    int step = 0;
    while (step < LOOP_MAX) {
      /* Arbitrary inputs each cycle */
      rtU.ia = nondet_double();
      rtU.ib = nondet_double();
      rtU.ic = nondet_double();
      __ESBMC_assume(rtU.ia >= -1.0e6 && rtU.ia <= 1.0e6);
      __ESBMC_assume(rtU.ib >= -1.0e6 && rtU.ib <= 1.0e6);
      __ESBMC_assume(rtU.ic >= -1.0e6 && rtU.ic <= 1.0e6);

      triplex_12B_step();

      if (rtY.FC == 0) {
        double med = median3(rtU.ia, rtU.ib, rtU.ic);
        __ESBMC_assert(rtY.sel_val == med, "RM-002: When FC == 0, sel_val must be the median of ia, ib, ic");
      }
      step++;
    }
  }
#endif /* VERIFY_PROPERTY_2 */

#ifdef VERIFY_PROPERTY_3
  {
    /* RM-003: Single-fail state good channel average
     * Verify: when FC is latched to {1,2,4}, sel_val equals the average of the two good channels:
     *   FC == 1 => (ia + ib)/2
     *   FC == 2 => (ia + ic)/2
     *   FC == 4 => (ib + ic)/2
     */
    /* Reset state */
    rtDW.Delay1_DSTATE[0] = 0;
    rtDW.Delay1_DSTATE[1] = 0;
    rtDW.Delay1_DSTATE[2] = 0;

    /* Build a latch condition first: persistent single-fail until prev PC > PClimit */
    int fail_case_code = nondet_int();
    __ESBMC_assume(fail_case_code == 4 || fail_case_code == 2 || fail_case_code == 1);
    int expected_fc = expected_fc_for_failcase(fail_case_code);

    rtU.PClimit = nondet_int();
    __ESBMC_assume(rtU.PClimit >= 1 && rtU.PClimit <= 4);
    rtU.Tlevel = nondet_double();
    __ESBMC_assume(rtU.Tlevel > 0.0 && rtU.Tlevel <= 1000.0);

    double base = nondet_double();
    __ESBMC_assume(base >= -1.0e6 && base <= 1.0e6);
    double d = nondet_double();
    __ESBMC_assume(d > rtU.Tlevel && d <= rtU.Tlevel + 1000.0);

    set_single_fail_inputs(base, d, fail_case_code);

    int max_steps = rtU.PClimit + 3;
    if (max_steps < 4) max_steps = 4;

    /* Run until latch happens */
    int step = 0;
    int prevPC = rtDW.Delay1_DSTATE[0];
    while (step < max_steps) {
      triplex_12B_step();
      if (prevPC > rtU.PClimit) break; /* latch should have occurred here */
      prevPC = rtY.PC;
      step++;
    }

    /* Ensure FC latched as expected (precondition for next checks) */
    __ESBMC_assert(rtY.FC == expected_fc, "RM-003: Precondition - FC must latch to expected code");

    /* Now vary inputs arbitrarily; sel_val must follow the average of the two good channels */
    int check_steps = LOOP_MAX;
    while (check_steps-- > 0) {
      /* Arbitrary values each cycle */
      rtU.ia = nondet_double();
      rtU.ib = nondet_double();
      rtU.ic = nondet_double();
      __ESBMC_assume(rtU.ia >= -1.0e6 && rtU.ia <= 1.0e6);
      __ESBMC_assume(rtU.ib >= -1.0e6 && rtU.ib <= 1.0e6);
      __ESBMC_assume(rtU.ic >= -1.0e6 && rtU.ic <= 1.0e6);

      triplex_12B_step();

      if (rtY.FC == 1) {
        __ESBMC_assert(rtY.sel_val == (rtU.ia + rtU.ib) / 2.0,
          "RM-003: FC==1 => sel_val must equal (ia + ib)/2");
      } else if (rtY.FC == 2) {
        __ESBMC_assert(rtY.sel_val == (rtU.ia + rtU.ic) / 2.0,
          "RM-003: FC==2 => sel_val must equal (ia + ic)/2");
      } else if (rtY.FC == 4) {
        __ESBMC_assert(rtY.sel_val == (rtU.ib + rtU.ic) / 2.0,
          "RM-003: FC==4 => sel_val must equal (ib + ic)/2");
      } else {
        __ESBMC_assert(0, "RM-003: FC left single-fail set unexpectedly");
      }
    }
  }
#endif /* VERIFY_PROPERTY_3 */

#ifdef VERIFY_PROPERTY_4
  {
    /* RM-004: Second failure in progress value retention
     * Verify: In single-fail state, when the remaining two good branches miscompare (> Tlevel),
     * the selected value should remain the previous value (retain).
     *
     * Note: Implementation comment indicates GCA is still used, which may violate this requirement.
     * This property checks if retention truly holds.
     */
    /* Reset state */
    rtDW.Delay1_DSTATE[0] = 0;
    rtDW.Delay1_DSTATE[1] = 0;
    rtDW.Delay1_DSTATE[2] = 0;

    /* First, drive a single-fail latch */
    int fail_case_code = nondet_int();
    __ESBMC_assume(fail_case_code == 4 || fail_case_code == 2 || fail_case_code == 1);
    int expected_fc = expected_fc_for_failcase(fail_case_code);

    rtU.PClimit = nondet_int();
    __ESBMC_assume(rtU.PClimit >= 1 && rtU.PClimit <= 4);
    rtU.Tlevel = nondet_double();
    __ESBMC_assume(rtU.Tlevel > 0.0 && rtU.Tlevel <= 1000.0);

    double base = nondet_double();
    __ESBMC_assume(base >= -1.0e6 && base <= 1.0e6);
    double d = nondet_double();
    __ESBMC_assume(d > rtU.Tlevel && d <= rtU.Tlevel + 1000.0);

    set_single_fail_inputs(base, d, fail_case_code);

    int max_steps = rtU.PClimit + 3;
    if (max_steps < 4) max_steps = 4;

    int step = 0;
    int prevPC = rtDW.Delay1_DSTATE[0];
    while (step < max_steps) {
      triplex_12B_step();
      if (prevPC > rtU.PClimit) break; /* latch point */
      prevPC = rtY.PC;
      step++;
    }

    __ESBMC_assert(rtY.FC == expected_fc, "RM-004: Precondition - FC must latch to expected code");

    /* Snapshot the selected value after latch */
    double retained_sel = rtY.sel_val;

    /* Now enforce second failure in progress: remaining good branches miscompare (>Tlevel) */
    double d2 = nondet_double();
    __ESBMC_assume(d2 > rtU.Tlevel && d2 <= rtU.Tlevel + 1000.0);

    int k = 0;
    while (k < 3) {
      /* Keep miscompare between remaining good branches; vary them each cycle */
      set_second_fail_in_progress(base, d2);
      triplex_12B_step();

      /* Property: selection should remain the previous value during second failure in progress */
      __ESBMC_assert(rtY.sel_val == retained_sel,
        "RM-004: During second failure in progress, sel_val must retain previous value (no change)");

      /* Do NOT update retained_sel; requirement expects it to remain constant */
      k++;
    }
  }
#endif /* VERIFY_PROPERTY_4 */

  return 0;
}
```