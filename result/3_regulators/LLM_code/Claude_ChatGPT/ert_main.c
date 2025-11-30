Below is an ESBMC-ready ert_main.c harness that drives regs_12B_step(), generates nondeterministic inputs each cycle, and verifies the 10 requirements using __ESBMC_assert with VERIFY_PROPERTY_N preprocessing macros. It also checks that states properly carry previous values across cycles for the height and pitch channels, without using static variables for previous loop information.

You can control which properties are verified by defining VERIFY_PROPERTY_1 through VERIFY_PROPERTY_10 at compile time (do not define them in this file). The harness verifies properties after each loop iteration and uses a while loop with a reasonable cycle count.

Save this as ert_main.c and compile/link it with the provided regs_12B.c and any data files that define rtConstP.

Code:
#include "regs_12B.h"

/* ESBMC nondet prototypes (names without prefixes as requested) */
double nondet_double(void);
_Bool nondet_bool(void);

/* Simple absolute value and saturation helpers */
static inline double dabs(double x) { return (x >= 0.0) ? x : -x; }
#define SAT(x, lo, hi) (((x) > (hi)) ? (hi) : (((x) < (lo)) ? (lo) : (x)))

/* Linear interpolation for 2-point table (used for transition_bvr) */
static inline double lookup_bvr(double u) {
  double x0 = rtConstP.transition_bvr_XData[0];
  double x1 = rtConstP.transition_bvr_XData[1];
  double y0 = rtConstP.transition_bvr_YData[0];
  double y1 = rtConstP.transition_bvr_YData[1];
  double m = (y1 - y0) / (x1 - x0);
  return y0 + m * (u - x0);
}

#ifndef MAX_STEPS
#define MAX_STEPS 150
#endif

#define EPS 1e-9

int main(void)
{
  /* Explicitly set known initial conditions for states (all zeros) */
  rtDW.UnitDelay_DSTATE = 0.0;
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay_DSTATE_o = 0.0;
  rtDW.UnitDelay1_DSTATE_k = 0.0;
  rtDW.Memory_PreviousInput = 0.0;

  /* Initialize model */
  regs_12B_initialize();

  /* Previous outputs for rate-limit checks */
  double prev_lcvdt = 0.0;
  double prev_mcvdt = 0.0;
  double prev_ncvdt = 0.0;
  double prev_xcvdt = 0.0;
  double prev_hcvdt = 0.0;

  /* Previous inputs to reconstruct previous error/integrator states */
  double prev_hcv_cmd = 0.0;
  double prev_zcv = 0.0;
  double prev_mcv_cmd = 0.0;
  double prev_mcv_fcs = 0.0;
  double prev_vtas = 0.0;

  /* Previous reconstructed integrator states */
  double prev_h_err_sat = 0.0; /* saturated height error used in previous cycle */
  double prev_h_int = 0.0;     /* rtb_Sum1 (height integrator output previous cycle) */

  double prev_m_err = 0.0;     /* pitch error previous cycle */
  double prev_m_int = 0.0;     /* rtb_Sum1_h (pitch integrator output previous cycle) */

  /* Consecutive-in-bound counters for static limit properties */
  int cnt_roll_static = 0;
  int cnt_pitch_static = 0;
  int cnt_yaw_static = 0;
  int cnt_axial_static = 0;
  int cnt_height_static = 0;

  _Bool first_step = 1;

  /* Run for a reasonable number of cycles */
  int step = 0;
  while (step < MAX_STEPS) {
    /* Generate nondeterministic inputs for this cycle */
    rtU.beta_adc_deg      = nondet_double();
    rtU.vtas_adc_kts      = nondet_double();
    rtU.lcv_cmd_fcs_dps   = nondet_double();
    rtU.hdg_des_deg       = nondet_double();
    rtU.mcv_cmd_fcs_dps   = nondet_double();
    rtU.alt_des_ft        = nondet_double();
    rtU.ncv_cmd_fcs_dps   = nondet_double();
    rtU.xcv_cmd_fcs_fps   = nondet_double();
    rtU.airspeed_des_fps  = nondet_double();
    rtU.hcv_cmd_fcs_fps   = nondet_double();
    rtU.lcv_fps_dps       = nondet_double();
    rtU.mcv_fcs_dps       = nondet_double();
    rtU.ncv_fcs_dps       = nondet_double();
    rtU.dcv_fcs_fps       = nondet_double();
    rtU.zcv_fcs_fps       = nondet_double();
    rtU.beta_dot          = nondet_double();

    /* Execute one step */
    regs_12B_step();

    /* Update consecutive counters for static limits */
#ifdef VERIFY_PROPERTY_1
    if (dabs(rtY.lcvdt_cmd_fcs_dps2) <= 50.0) cnt_roll_static++; else cnt_roll_static = 0;
#endif
#ifdef VERIFY_PROPERTY_2
    if (dabs(rtY.mcvdt_cmd_fcs_dps2) <= 50.0) cnt_pitch_static++; else cnt_pitch_static = 0;
#endif
#ifdef VERIFY_PROPERTY_3
    if (dabs(rtY.ncvdt_cmd_fcs_dps2) <= 50.0) cnt_yaw_static++; else cnt_yaw_static = 0;
#endif
#ifdef VERIFY_PROPERTY_4
    if (dabs(rtY.xcvdt_cmd_fcs_fps2) <= 32.0) cnt_axial_static++; else cnt_axial_static = 0;
#endif
#ifdef VERIFY_PROPERTY_5
    if (dabs(rtY.hcvdt_cmd_fcs_fps2) <= 32.0) cnt_height_static++; else cnt_height_static = 0;
#endif

    /* Static limit assertions: persistence over 100 consecutive cycles */
#ifdef VERIFY_PROPERTY_1
    if (step >= 99) {
      __ESBMC_assert(cnt_roll_static >= 100, "REQ1: |lcvdt_cmd_fcs_dps2| <= 50 for 100 consecutive cycles");
    }
#endif
#ifdef VERIFY_PROPERTY_2
    if (step >= 99) {
      __ESBMC_assert(cnt_pitch_static >= 100, "REQ2: |mcvdt_cmd_fcs_dps2| <= 50 for 100 consecutive cycles");
    }
#endif
#ifdef VERIFY_PROPERTY_3
    if (step >= 99) {
      __ESBMC_assert(cnt_yaw_static >= 100, "REQ3: |ncvdt_cmd_fcs_dps2| <= 50 for 100 consecutive cycles");
    }
#endif
#ifdef VERIFY_PROPERTY_4
    if (step >= 99) {
      __ESBMC_assert(cnt_axial_static >= 100, "REQ4: |xcvdt_cmd_fcs_fps2| <= 32 for 100 consecutive cycles");
    }
#endif
#ifdef VERIFY_PROPERTY_5
    if (step >= 99) {
      __ESBMC_assert(cnt_height_static >= 100, "REQ5: |hcvdt_cmd_fcs_fps2| <= 32 for 100 consecutive cycles");
    }
#endif

    /* Rate limit assertions: compare with previous outputs */
    if (!first_step) {
#ifdef VERIFY_PROPERTY_6
      __ESBMC_assert(dabs(rtY.lcvdt_cmd_fcs_dps2 - prev_lcvdt) <= 50.0 + EPS,
                     "REQ6: |lcvdt_cmd_fcs_dps2[n] - lcvdt_cmd_fcs_dps2[n-1]| <= 50");
#endif
#ifdef VERIFY_PROPERTY_7
      __ESBMC_assert(dabs(rtY.mcvdt_cmd_fcs_dps2 - prev_mcvdt) <= 50.0 + EPS,
                     "REQ7: |mcvdt_cmd_fcs_dps2[n] - mcvdt_cmd_fcs_dps2[n-1]| <= 50");
#endif
#ifdef VERIFY_PROPERTY_8
      __ESBMC_assert(dabs(rtY.ncvdt_cmd_fcs_dps2 - prev_ncvdt) <= 50.0 + EPS,
                     "REQ8: |ncvdt_cmd_fcs_dps2[n] - ncvdt_cmd_fcs_dps2[n-1]| <= 50");
#endif
#ifdef VERIFY_PROPERTY_9
      __ESBMC_assert(dabs(rtY.xcvdt_cmd_fcs_fps2 - prev_xcvdt) <= 32.0 + EPS,
                     "REQ9: |xcvdt_cmd_fcs_fps2[n] - xcvdt_cmd_fcs_fps2[n-1]| <= 32");
#endif
#ifdef VERIFY_PROPERTY_10
      __ESBMC_assert(dabs(rtY.hcvdt_cmd_fcs_fps2 - prev_hcvdt) <= 32.0 + EPS,
                     "REQ10: |hcvdt_cmd_fcs_fps2[n] - hcvdt_cmd_fcs_fps2[n-1]| <= 32");
#endif
    }

    /* State retention and update checks for Height channel */
#ifdef VERIFY_PROPERTY_5
    {
      /* Current cycle height error (saturated): rtb_CVdt_cmd after saturation and zero memory gain */
      double curr_h_err = SAT(rtU.hcv_cmd_fcs_fps - (-rtU.zcv_fcs_fps), -1.0, 1.0);
      /* Current integrator output reconstructed: y = 0.25*s1 + err => s1 = 4*(y - err) */
      double curr_h_int = 4.0 * (rtY.hcvdt_cmd_fcs_fps2 - curr_h_err);

      /* Check that states updated to current cycle internal values */
      __ESBMC_assert(dabs(rtDW.UnitDelay_DSTATE - curr_h_err) <= EPS,
                     "HEIGHT STATE UPDATE: UnitDelay_DSTATE equals current saturated height error");
      __ESBMC_assert(dabs(rtDW.UnitDelay1_DSTATE - curr_h_int) <= EPS,
                     "HEIGHT STATE UPDATE: UnitDelay1_DSTATE equals current height integrator output");
      __ESBMC_assert(dabs(rtDW.Memory_PreviousInput - curr_h_int) <= EPS,
                     "HEIGHT STATE UPDATE: Memory_PreviousInput equals current height integrator output");

      /* If not the first step, verify that the new integrator state uses previous states as per Tustin */
      if (!first_step) {
        /* Predicted integrator next value using previous states (Tustin trapezoidal), with saturation to ±9999 */
        double pred_h_int = SAT((curr_h_err + prev_h_err_sat) * 0.5 * 0.01 + prev_h_int, -9999.0, 9999.0);
        __ESBMC_assert(dabs(curr_h_int - pred_h_int) <= EPS,
                       "HEIGHT STATE RETENTION: Tustin integrator uses previous height error and integrator state");
      }

      /* Save for next iteration retention check */
      prev_h_err_sat = curr_h_err;
      prev_h_int     = curr_h_int;
    }
#endif

    /* State retention and update checks for Pitch channel */
#ifdef VERIFY_PROPERTY_2
    {
      /* Current pitch error */
      double curr_m_err = rtU.mcv_cmd_fcs_dps - rtU.mcv_fcs_dps;
      /* Transition blend gain (linear extrapolation across two points) */
      double g_bvr = lookup_bvr(rtU.vtas_adc_kts);
      /* Reconstruct current pitch integrator output: y = g*err + 5*s1 => s1 = (y - g*err)/5 */
      double curr_m_int = (rtY.mcvdt_cmd_fcs_dps2 - g_bvr * curr_m_err) / 5.0;

      /* Check that pitch states updated to current cycle internal values */
      __ESBMC_assert(dabs(rtDW.UnitDelay_DSTATE_o - curr_m_err) <= EPS,
                     "PITCH STATE UPDATE: UnitDelay_DSTATE_o equals current pitch error");
      __ESBMC_assert(dabs(rtDW.UnitDelay1_DSTATE_k - curr_m_int) <= EPS,
                     "PITCH STATE UPDATE: UnitDelay1_DSTATE_k equals current pitch integrator output");

      /* If not the first step, verify Tustin integrator uses previous cycle's states */
      if (!first_step) {
        double pred_m_int = SAT((curr_m_err + prev_m_err) * 0.5 * 0.01 + prev_m_int, -9999.0, 9999.0);
        __ESBMC_assert(dabs(curr_m_int - pred_m_int) <= EPS,
                       "PITCH STATE RETENTION: Tustin integrator uses previous pitch error and integrator state");
      }

      /* Save for next iteration */
      prev_m_err = curr_m_err;
      prev_m_int = curr_m_int;
    }
#endif

    /* Save previous outputs and inputs for next iteration */
    prev_lcvdt = rtY.lcvdt_cmd_fcs_dps2;
    prev_mcvdt = rtY.mcvdt_cmd_fcs_dps2;
    prev_ncvdt = rtY.ncvdt_cmd_fcs_dps2;
    prev_xcvdt = rtY.xcvdt_cmd_fcs_fps2;
    prev_hcvdt = rtY.hcvdt_cmd_fcs_fps2;

    prev_hcv_cmd = rtU.hcv_cmd_fcs_fps;
    prev_zcv     = rtU.zcv_fcs_fps;
    prev_mcv_cmd = rtU.mcv_cmd_fcs_dps;
    prev_mcv_fcs = rtU.mcv_fcs_dps;
    prev_vtas    = rtU.vtas_adc_kts;

    first_step = 0;
    step++;
  }

  return 0;
}

Notes:
- Properties are controlled with macros VERIFY_PROPERTY_1 to VERIFY_PROPERTY_10. Do not define them in the file; define them via compiler flags, e.g., -DVERIFY_PROPERTY_1.
- The while loop runs for MAX_STEPS cycles (default 150). You can change with -DMAX_STEPS=N.
- Static limit properties (REQ1–REQ5) maintain per-output consecutive counters and assert the “100 consecutive cycles” condition once step >= 100.
- Rate limit properties (REQ6–REQ10) compare current and previous outputs each cycle and assert the per-step rate limits.
- State retention and update checks are embedded under VERIFY_PROPERTY_5 (height) and VERIFY_PROPERTY_2 (pitch). They:
  - Confirm UnitDelay and Memory states equal the internal values computed in the current step.
  - Confirm, on subsequent steps, that the Tustin integrator state equals the predicted value computed from the previous error and integrator state.
- No static variables are used to store previous loop information; simple local variables are used instead.
- The saturation and interpolation are consistent with the generated code: height error is saturated to [-1, 1]; Tustin integrator outputs are saturated to ±9999; the pitch blend gain is computed via a 2-point linear interpolation (transition_bvr).