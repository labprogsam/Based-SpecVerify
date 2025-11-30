Here is an assertion-based ert_main.c that uses ESBMC to verify the listed requirements. It uses __ESBMC_assert, nondet generators, and preprocessing macros VERIFY_PROPERTY_1A ... VERIFY_PROPERTY_4B to control which requirements are checked. It verifies outputs after every loop iteration and also checks that previous-state variables are updated correctly each step (UnitDelay_DSTATE keeps previous cmd; UnitDelay1_DSTATE keeps previous yout). No static variables are used to store previous loop information.

Save as ert_main.c.

#include "integrator_12B.h"
#include <math.h>

/* ESBMC nondeterministic generators (no prefix, use _Bool for boolean) */
_Bool nondet_bool(void);
int nondet_int(void);
double nondet_double(void);

/* Small epsilon for floating comparisons */
#define EPS 1e-12

/* Helper to reinitialize model states between property runs */
static void reset_model_states(void)
{
  rtDW.UnitDelay_DSTATE = 0.0;
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtY.yout = 0.0;
}

/* Helper to assert state persistence after each step */
static void assert_state_persistence(void)
{
  __ESBMC_assert(rtDW.UnitDelay_DSTATE == rtU.cmd,
                 "State persistence: UnitDelay_DSTATE must equal current cmd after step");
  __ESBMC_assert(rtDW.UnitDelay1_DSTATE == rtY.yout,
                 "State persistence: UnitDelay1_DSTATE must equal yout after step");
}

int main(void)
{
  integrator_12B_initialize();

  /* Requirement 1a: Reset with bounded IC (BL <= ic <= TL) */
#ifdef VERIFY_PROPERTY_1A
  reset_model_states();
  {
    int count = 5;
    while (count > 0) {
      /* Prepare inputs satisfying BL <= ic <= TL and reset = true */
      double base = nondet_double();
      double delta = fabs(nondet_double());
      rtU.BL = base;
      rtU.TL = base + delta; /* TL >= BL */
      rtU.ic = rtU.BL + 0.5 * (rtU.TL - rtU.BL); /* in the middle of bounds */
      rtU.T = nondet_double(); /* unused in reset path */
      rtU.cmd = nondet_double();
      rtU.reset = (_Bool)1;

      /* Cache previous states for any checks if needed */
      double prevCmd = rtDW.UnitDelay_DSTATE;
      double prevY = rtDW.UnitDelay1_DSTATE;

      integrator_12B_step();

      __ESBMC_assert(fabs(rtY.yout - rtU.ic) <= EPS,
                     "Req 1a: On reset with BL <= ic <= TL, output must equal ic");

      assert_state_persistence();

      (void)prevCmd; (void)prevY; /* silence unused warnings */
      count--;
    }
  }
#endif

  /* Requirement 1b: Reset with IC >= TL (TL >= BL) => yout = TL */
#ifdef VERIFY_PROPERTY_1B
  reset_model_states();
  {
    int count = 5;
    while (count > 0) {
      double base = nondet_double();
      double delta = fabs(nondet_double()); /* >= 0 */
      rtU.BL = base;
      rtU.TL = base + delta; /* TL >= BL */
      rtU.ic = rtU.TL + fabs(nondet_double()); /* ic >= TL */
      rtU.T = nondet_double();
      rtU.cmd = nondet_double();
      rtU.reset = (_Bool)1;

      integrator_12B_step();

      __ESBMC_assert(fabs(rtY.yout - rtU.TL) <= EPS,
                     "Req 1b: On reset with ic >= TL (TL >= BL), output must equal TL");

      assert_state_persistence();
      count--;
    }
  }
#endif

  /* Requirement 1c: Reset with IC <= BL (TL >= BL) => yout = BL */
#ifdef VERIFY_PROPERTY_1C
  reset_model_states();
  {
    int count = 5;
    while (count > 0) {
      double base = nondet_double();
      double delta = fabs(nondet_double()); /* >= 0 */
      rtU.BL = base;
      rtU.TL = base + delta; /* TL >= BL */
      rtU.ic = rtU.BL - fabs(nondet_double()); /* ic <= BL */
      rtU.T = nondet_double();
      rtU.cmd = nondet_double();
      rtU.reset = (_Bool)1;

      integrator_12B_step();

      __ESBMC_assert(fabs(rtY.yout - rtU.BL) <= EPS,
                     "Req 1c: On reset with ic <= BL (TL >= BL), output must equal BL");

      assert_state_persistence();
      count--;
    }
  }
#endif

  /* Requirement 1d: Reset with IC >= BL (TL < BL) => yout = BL */
#ifdef VERIFY_PROPERTY_1D
  reset_model_states();
  {
    int count = 5;
    while (count > 0) {
      /* Construct TL < BL */
      double base = nondet_double();
      double delta = fabs(nondet_double());
      rtU.TL = base;
      rtU.BL = base + delta; /* BL > TL */
      rtU.ic = rtU.BL + fabs(nondet_double()); /* ic >= BL */
      rtU.T = nondet_double();
      rtU.cmd = nondet_double();
      rtU.reset = (_Bool)1;

      integrator_12B_step();

      __ESBMC_assert(fabs(rtY.yout - rtU.BL) <= EPS,
                     "Req 1d: On reset with ic >= BL (TL < BL), output must equal BL");

      assert_state_persistence();
      count--;
    }
  }
#endif

  /* Requirement 1e: Reset with IC <= TL (TL < BL) => yout = TL */
#ifdef VERIFY_PROPERTY_1E
  reset_model_states();
  {
    int count = 5;
    while (count > 0) {
      /* Construct TL < BL */
      double base = nondet_double();
      double delta = fabs(nondet_double());
      rtU.TL = base;
      rtU.BL = base + delta; /* BL > TL */
      rtU.ic = rtU.TL - fabs(nondet_double()); /* ic <= TL */
      rtU.T = nondet_double();
      rtU.cmd = nondet_double();
      rtU.reset = (_Bool)1;

      integrator_12B_step();

      __ESBMC_assert(fabs(rtY.yout - rtU.TL) <= EPS,
                     "Req 1e: On reset with ic <= TL (TL < BL), output must equal TL");

      assert_state_persistence();
      count--;
    }
  }
#endif

  /* Requirement 2a: Output bounded when TL >= BL (always) */
#ifdef VERIFY_PROPERTY_2A
  reset_model_states();
  {
    int count = 8;
    while (count > 0) {
      /* Ensure TL >= BL */
      double base = nondet_double();
      double delta = fabs(nondet_double());
      rtU.BL = base;
      rtU.TL = base + delta; /* TL >= BL */

      rtU.reset = nondet_bool();
      rtU.ic = nondet_double();
      rtU.T = nondet_double();
      rtU.cmd = nondet_double();

      integrator_12B_step();

      if (rtU.TL >= rtU.BL) {
        __ESBMC_assert(rtY.yout <= rtU.TL + EPS && rtY.yout + EPS >= rtU.BL,
                       "Req 2a: Output must remain within [BL, TL] when TL >= BL");
      }

      assert_state_persistence();
      count--;
    }
  }
#endif

  /* Requirement 2b: Output bounded when TL < BL (always) */
#ifdef VERIFY_PROPERTY_2B
  reset_model_states();
  {
    int count = 8;
    while (count > 0) {
      /* Ensure TL < BL */
      double base = nondet_double();
      double delta = fabs(nondet_double());
      rtU.TL = base;
      rtU.BL = base + delta; /* BL > TL */

      rtU.reset = nondet_bool();
      rtU.ic = nondet_double();
      rtU.T = nondet_double();
      rtU.cmd = nondet_double();

      integrator_12B_step();

      if (rtU.TL < rtU.BL) {
        __ESBMC_assert(rtY.yout <= rtU.BL + EPS && rtY.yout + EPS >= rtU.TL,
                       "Req 2b: Output must remain within [TL, BL] when TL < BL");
      }

      assert_state_persistence();
      count--;
    }
  }
#endif

  /* Requirement 3: Normal operation equation (unsaturated), TL >= BL, reset == false */
#ifdef VERIFY_PROPERTY_3
  reset_model_states();
  {
    int count = 8;
    while (count > 0) {
      /* Wide bounds to reduce saturation chance */
      rtU.BL = -1e6;
      rtU.TL =  1e6;

      /* Choose arbitrary values */
      rtU.reset = (_Bool)0;
      rtU.T = nondet_double();
      rtU.cmd = nondet_double();
      rtU.ic = nondet_double(); /* not used when reset == false */

      /* Snapshot previous states BEFORE step */
      double prevCmd = rtDW.UnitDelay_DSTATE;
      double prevY   = rtDW.UnitDelay1_DSTATE;

      /* Compute expected unsaturated output using Tustin formula */
      double r = (rtU.cmd + prevCmd) * 0.5 * rtU.T + prevY;

      integrator_12B_step();

      /* Check only when the preconditions are satisfied and r is within bounds */
      if ((!rtU.reset) && (rtU.TL >= rtU.BL) && (r <= rtU.TL + EPS) && (r + EPS >= rtU.BL)) {
        __ESBMC_assert(fabs(rtY.yout - r) <= EPS,
                       "Req 3: Unsaturated normal operation must match Tustin formula");
      }

      assert_state_persistence();
      (void)prevCmd; (void)prevY;
      count--;
    }
  }
#endif

  /* Requirement 4a: Constant input integration test */
#ifdef VERIFY_PROPERTY_4A
  reset_model_states();
  {
    /* Setup: cmd = 1.0, T = 0.1, reset = false, wide bounds to avoid saturation */
    rtU.BL = -1e6;
    rtU.TL =  1e6;
    rtU.T = 0.1;
    rtU.reset = (_Bool)0;
    rtU.ic = 0.0; /* not used with reset=false */

    int i = 0;
    while (i <= 100) {
      rtU.cmd = 1.0;

      integrator_12B_step();

      /* Expected integral of 1.0 is t = i * 0.1 */
      double t = (double)i * 0.1;
      double expected = t;

      __ESBMC_assert(fabs(rtY.yout - expected) <= 0.1 + 1e-9,
                     "Req 4a: |yout - t| must be <= 0.1 for constant input test");

      assert_state_persistence();
      i++;
    }
  }
#endif

  /* Requirement 4b: Cosine input integration test */
#ifdef VERIFY_PROPERTY_4B
  reset_model_states();
  {
    /* Setup: cmd = cos(t), T = 0.1, reset = false, wide bounds to avoid saturation */
    rtU.BL = -1e6;
    rtU.TL =  1e6;
    rtU.T = 0.1;
    rtU.reset = (_Bool)0;
    rtU.ic = 0.0; /* not used with reset=false */

    int i = 0;
    double t = 0.0;
    while (i <= 100) {
      rtU.cmd = cos(t);

      integrator_12B_step();

      /* Expected integral of cos(t) is sin(t) */
      double expected = sin(t);

      __ESBMC_assert(fabs(rtY.yout - expected) <= 0.1 + 1e-9,
                     "Req 4b: |yout - sin(t)| must be <= 0.1 for cosine integration test");

      assert_state_persistence();

      t += 0.1;
      i++;
    }
  }
#endif

  return 0;
}

Notes:
- Use -DVERIFY_PROPERTY_1A (or 1B, 1C, 1D, 1E, 2A, 2B, 3, 4A, 4B) to select which requirement to verify with ESBMC.
- The program verifies state persistence after each step in every block, per your request to check whether values keep previous state.
- No static variables are used for previous-state information; local variables are used where needed.
- For floating comparisons that should be exact in the model (like reset paths and unsaturated equality), a very small EPS is used to be robust to double rounding.
- For 4a and 4b, a tolerance of 0.1 is enforced as requested.