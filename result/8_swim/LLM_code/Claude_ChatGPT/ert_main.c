/* ert_main.c
 * ESBMC verification harness with assertion-based properties
 * Requirements covered:
 *  - Req 1: SWIM_CalAirspeedmin_kts calculation (CAT I vs CAT III)
 *  - Req 2a: SWIM_Qcmin_lbspft2 calculation
 *  - Req 2b: SWIM_ASWarningAllowed state machine and persistence
 *  - Req 2c: swimGet_AgcasLowSpeedWarn state machine and persistence
 *  - Disable threshold output: swimGet_QcMinDisable_lbspft2 = Qcmin - 28.72
 *  - State update timing: UnitDelay states updated after step()
 *
 * Control properties via -DVERIFY_PROPERTY_N (do not define in this file).
 */

#include "swim_12B.h"
#include "rtwtypes.h"
#include <math.h>

/* Non-deterministic input generators (ESBMC provides these) */
extern double nondet_double(void);
extern int nondet_int(void);
extern _Bool nondet_bool(void);

/* ESBMC assertion */
extern void __ESBMC_assert(_Bool, const char*);

/* ESBMC assumption (optional constraints) */
extern void __ESBMC_assume(_Bool);

static _Bool approx_equal(double a, double b, double eps) {
  double d = a - b;
  if (d < 0) d = -d;
  return d <= eps;
}

int main(void)
{
  /* Initialize model and states */
  swim_12B_initialize();

  /* Explicitly set initial UnitDelay states (as per requirement defaults) */
  rtDW.UnitDelay_DSTATE = 0.0;     /* previous SWIM_ASWarningAllowed */
  rtDW.UnitDelay_DSTATE_d = 0.0;   /* previous swimGet_AgcasLowSpeedWarn */

  /* Local "previous state" mirrors to check persistence behavior */
  double prev_ASWarningAllowed = 0.0;
  double prev_LowSpeedWarn = 0.0;

  /* Loop bounds and runner */
  int steps = nondet_int();
  __ESBMC_assume(steps >= 1 && steps <= 5); /* reasonable bounded exploration */

  int i = 0;
  while (i < steps) {
    /* At step start, UnitDelay must reflect previous outputs */
    #ifdef VERIFY_PROPERTY_6
    __ESBMC_assert(approx_equal(rtDW.UnitDelay_DSTATE, prev_ASWarningAllowed, 1e-12),
                   "P6a: UnitDelay_DSTATE must equal previous SWIM_ASWarningAllowed");
    __ESBMC_assert(approx_equal(rtDW.UnitDelay_DSTATE_d, prev_LowSpeedWarn, 1e-12),
                   "P6b: UnitDelay_DSTATE_d must equal previous swimGet_AgcasLowSpeedWarn");
    #endif

    /* Drive inputs */
    int catPos = nondet_int();                     /* 0 = CAT I, non-zero = CAT III */
    double gw_lbs = nondet_double();               /* Aircraft gross weight (lbs) */
    double impact = nondet_double();               /* E_AI_IMPACT_PRESSURE */
    _Bool gearAltFlap = nondet_bool();             /* diGet_S_LandingGearAltFlap */
    _Bool monitorEna = nondet_bool();              /* muxGet_T_Mux_AirspeedMonitorEna */
    _Bool cvWarn = nondet_bool();                  /* cvGet_V_AgcasLowSpeedWarn */
    _Bool interlocks = nondet_bool();              /* olcGet_AgcasInterlocks */

    /* Constrain physical ranges to avoid NaN in sqrt and unreasonable values */
    __ESBMC_assume(gw_lbs >= 0.0 && gw_lbs <= 2.0e6);
    __ESBMC_assume(impact > -1.0e6 && impact < 1.0e6);

    rtU.diGet_S_CatSwitchPosition = catPos;
    rtU.muxGet_T_Mux_AircraftGrossWeigh = gw_lbs;
    rtU.E_AI_IMPACT_PRESSURE = impact;
    rtU.diGet_S_LandingGearAltFlap = (boolean_T)gearAltFlap;
    rtU.muxGet_T_Mux_AirspeedMonitorEna = (boolean_T)monitorEna;
    rtU.cvGet_V_AgcasLowSpeedWarn = (boolean_T)cvWarn;
    rtU.olcGet_AgcasInterlocks = (boolean_T)interlocks;

    /* Compute expected values based on the specification and previous state */
    double expCalAirspeed;
    if (catPos == 0) {
      expCalAirspeed = 1.25921 * sqrt(gw_lbs) + 10.0;
    } else {
      expCalAirspeed = 1.33694 * sqrt(gw_lbs) + 10.0;
    }

    double expQcmin = ((0.020306 * expCalAirspeed - 2.0906) + 0.1) * 70.7184;
    double expDisableThresh = expQcmin - 28.72;

    double expASAllowed;
    if (!gearAltFlap) {
      expASAllowed = 0.0;
    } else if (gearAltFlap && (impact > expQcmin)) {
      expASAllowed = 1.0;
    } else {
      expASAllowed = prev_ASWarningAllowed; /* hold previous */
    }

    double expLowSpeedWarn;
    if ((monitorEna || cvWarn) && (!interlocks) && (expASAllowed == 1.0)) {
      expLowSpeedWarn = (impact < expDisableThresh) ? 1.0 : 0.0;
    } else {
      expLowSpeedWarn = prev_LowSpeedWarn; /* hold previous */
    }

    /* Run one step of the system */
    swim_12B_step();

    /* Verify Requirement 1: SWIM_CalAirspeedmin_kts calculation */
    #ifdef VERIFY_PROPERTY_1
    __ESBMC_assert(
      approx_equal(rtY.SWIM_CalAirspeedmin_kts, expCalAirspeed, 1e-9),
      "P1: SWIM_CalAirspeedmin_kts must match CAT-based formula");
    #endif

    /* Verify Requirement 2a: SWIM_Qcmin_lbspft2 calculation */
    #ifdef VERIFY_PROPERTY_2
    __ESBMC_assert(
      approx_equal(rtY.SWIM_Qcmin_lbspft2, expQcmin, 1e-9),
      "P2: SWIM_Qcmin_lbspft2 must match Qcmin formula from CalAirspeed");
    #endif

    /* Verify Disable threshold output */
    #ifdef VERIFY_PROPERTY_3
    __ESBMC_assert(
      approx_equal(rtY.swimGet_QcMinDisable_lbspft2, expDisableThresh, 1e-9),
      "P3: swimGet_QcMinDisable_lbspft2 must be (SWIM_Qcmin_lbspft2 - 28.72)");
    #endif

    /* Verify Requirement 2b: SWIM_ASWarningAllowed logic and persistence */
    #ifdef VERIFY_PROPERTY_4
    __ESBMC_assert(
      approx_equal(rtY.SWIM_ASWarningAllowed, expASAllowed, 1e-12),
      "P4a: SWIM_ASWarningAllowed must follow specified state machine");

    /* Persistence check: if neither reset nor set conditions, value must hold previous */
    _Bool hold_cond_AS =
      (gearAltFlap && !(impact > expQcmin)); /* else branch taken */
    if (hold_cond_AS) {
      __ESBMC_assert(
        approx_equal(rtY.SWIM_ASWarningAllowed, prev_ASWarningAllowed, 1e-12),
        "P4b: SWIM_ASWarningAllowed must hold previous state when else-branch");
    }
    #endif

    /* Verify Requirement 2c: swimGet_AgcasLowSpeedWarn logic and persistence */
    #ifdef VERIFY_PROPERTY_5
    __ESBMC_assert(
      approx_equal(rtY.swimGet_AgcasLowSpeedWarn, expLowSpeedWarn, 1e-12),
      "P5a: swimGet_AgcasLowSpeedWarn must follow specified state machine");

    /* Persistence check: when gating conditions not met, must hold previous */
    _Bool gating_met = ((monitorEna || cvWarn) && (!interlocks) && (expASAllowed == 1.0));
    if (!gating_met) {
      __ESBMC_assert(
        approx_equal(rtY.swimGet_AgcasLowSpeedWarn, prev_LowSpeedWarn, 1e-12),
        "P5b: swimGet_AgcasLowSpeedWarn must hold previous state when gating not met");
    }
    #endif

    /* Verify UnitDelay states updated after step */
    #ifdef VERIFY_PROPERTY_6
    __ESBMC_assert(
      approx_equal(rtDW.UnitDelay_DSTATE, rtY.SWIM_ASWarningAllowed, 1e-12),
      "P6c: UnitDelay_DSTATE must be updated to SWIM_ASWarningAllowed after step");
    __ESBMC_assert(
      approx_equal(rtDW.UnitDelay_DSTATE_d, rtY.swimGet_AgcasLowSpeedWarn, 1e-12),
      "P6d: UnitDelay_DSTATE_d must be updated to swimGet_AgcasLowSpeedWarn after step");
    #endif

    /* Cross-check expected model coherence (optional sanity) */
    #ifdef VERIFY_PROPERTY_7
    /* If CAT I then coefficient must be 1.25921; else 1.33694 */
    double coeff = (catPos == 0) ? 1.25921 : 1.33694;
    __ESBMC_assert(
      approx_equal(rtY.SWIM_CalAirspeedmin_kts, coeff * sqrt(gw_lbs) + 10.0, 1e-9),
      "P7: Cat-switch mapping coherence");
    #endif

    /* Update previous state mirrors for next iteration */
    prev_ASWarningAllowed = rtY.SWIM_ASWarningAllowed;
    prev_LowSpeedWarn = rtY.swimGet_AgcasLowSpeedWarn;

    i++;
  }

  return 0;
}