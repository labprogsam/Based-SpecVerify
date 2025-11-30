/*
 * ert_main.c - ESBMC verification harness for fsm_12B_global
 *
 * Notes:
 * - Uses __ESBMC_assert for properties guarded by VERIFY_PROPERTY_N
 * - Uses nondet_bool for unconstrained inputs when appropriate
 * - Executes fsm_12B_global_step() in a while loop and asserts after steps
 * - No static variables are used to store previous cycle info
 * - Verifies UnitDelay variables carry previous cycle output values
 */

#include "fsm_12B_global.h"

/* ESBMC intrinsics */
extern _Bool nondet_bool(void);
extern void __ESBMC_assert(_Bool, const char*);

int main(void)
{
  /* Common locals */
  int i;
  int max_steps;
  real_T exp_next_AP_state;     /* expected next UnitDelay_DSTATE (AP) */
  real_T exp_next_SEN_state;    /* expected next UnitDelay1_DSTATE (Sensor) */
  _Bool  exp_next_good;         /* expected next UnitDelay2_DSTATE (good) */

#ifdef VERIFY_PROPERTY_1
  /* Requirement 1:
     Exceeding sensor limits shall latch an autopilot pullup when pilot not in control and system supported without failures.
     Multi-cycle scenario:
       - Cycle 0: move AP to NOMINAL with good == true
       - Cycle 1: limits = true -> sensor goes FAULT; good becomes false next
       - Cycle 2: AP sees good == false -> AP -> MANEUVER, pullup == true
  */
  fsm_12B_global_initialize();

  /* Initialize states explicitly */
  rtDW.UnitDelay_DSTATE  = 0.0; /* AP TRANSITION */
  rtDW.UnitDelay1_DSTATE = 0.0; /* Sensor NOMINAL */
  rtDW.UnitDelay2_DSTATE = 1;   /* good = true */

  /* Inputs per requirement */
  rtU.standby   = 0;  /* pilot not in control */
  rtU.supported = 1;  /* system supported */
  rtU.apfail    = 0;  /* no failure */
  rtU.limits    = 0;  /* start within limits */

  max_steps = 3;
  exp_next_AP_state  = rtDW.UnitDelay_DSTATE;
  exp_next_SEN_state = rtDW.UnitDelay1_DSTATE;
  exp_next_good      = rtDW.UnitDelay2_DSTATE;

  i = 0;
  while (i < max_steps) {
    if (i == 1) {
      rtU.limits = 1; /* Exceed limits at cycle 1 */
    }

    fsm_12B_global_step();

    if (i > 0) {
      __ESBMC_assert(rtDW.UnitDelay_DSTATE  == exp_next_AP_state,  "P14: AP UnitDelay should equal previous cycle AP Merge");
      __ESBMC_assert(rtDW.UnitDelay1_DSTATE == exp_next_SEN_state, "P14: SEN UnitDelay should equal previous cycle SEN Merge");
      __ESBMC_assert(rtDW.UnitDelay2_DSTATE == exp_next_good,      "P14: GOOD UnitDelay should equal previous cycle computed good");
    }

    exp_next_AP_state  = rtDW.Merge;
    exp_next_SEN_state = rtDW.Merge_g;
    exp_next_good      = (rtDW.Merge_g != 2.0);

    if (i == 2) {
      __ESBMC_assert(rtY.pullup == 1, "Property 1 failed: pullup not asserted by expected cycle");
      __ESBMC_assert(rtY.STATE  == 2.0, "Property 1 failed: AP not in MANEUVER when limits exceeded under supported/no-fail/no-standby");
    }
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_2
  /* Requirement 2: AP TRANSITION -> STANDBY when standby == true (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 0.0; /* TRANSITION */
  rtDW.UnitDelay1_DSTATE = 0.0; /* Sensor NOMINAL */
  rtDW.UnitDelay2_DSTATE = 1;   /* good = true */

  rtU.standby   = 1;
  rtU.supported = nondet_bool();
  rtU.apfail    = nondet_bool();
  rtU.limits    = nondet_bool();

  max_steps = 2;
  exp_next_AP_state  = rtDW.UnitDelay_DSTATE;
  exp_next_SEN_state = rtDW.UnitDelay1_DSTATE;
  exp_next_good      = rtDW.UnitDelay2_DSTATE;

  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();

    if (i == 0) {
      __ESBMC_assert(rtY.STATE == 3.0, "Property 2 failed: AP not STANDBY when standby == true from TRANSITION");
    }

    if (i > 0) {
      __ESBMC_assert(rtDW.UnitDelay_DSTATE  == exp_next_AP_state,  "P14: AP UnitDelay mismatch");
      __ESBMC_assert(rtDW.UnitDelay1_DSTATE == exp_next_SEN_state, "P14: SEN UnitDelay mismatch");
      __ESBMC_assert(rtDW.UnitDelay2_DSTATE == exp_next_good,      "P14: GOOD UnitDelay mismatch");
    }

    exp_next_AP_state  = rtDW.Merge;
    exp_next_SEN_state = rtDW.Merge_g;
    exp_next_good      = (rtDW.Merge_g != 2.0);
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_3
  /* Requirement 3: AP TRANSITION -> NOMINAL when supported == true and good == true (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 0.0; /* TRANSITION */
  rtDW.UnitDelay1_DSTATE = 0.0; /* Sensor NOMINAL */
  rtDW.UnitDelay2_DSTATE = 1;   /* good = true */
  rtU.standby   = 0;
  rtU.supported = 1;
  rtU.apfail    = nondet_bool();
  rtU.limits    = nondet_bool();

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.STATE == 1.0, "Property 3 failed: AP not NOMINAL when supported && good from TRANSITION");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_4
  /* Requirement 4: AP NOMINAL -> MANEUVER when good == false (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 1.0; /* NOMINAL */
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay2_DSTATE = 0;   /* good = false */
  rtU.standby   = 0;
  rtU.supported = nondet_bool();
  rtU.apfail    = nondet_bool();
  rtU.limits    = nondet_bool();

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.STATE == 2.0, "Property 4 failed: AP not MANEUVER when good == false from NOMINAL");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_5
  /* Requirement 5: AP NOMINAL -> STANDBY when standby == true (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 1.0; /* NOMINAL */
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay2_DSTATE = 1;
  rtU.standby   = 1;
  rtU.supported = nondet_bool();
  rtU.apfail    = nondet_bool();
  rtU.limits    = nondet_bool();

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.STATE == 3.0, "Property 5 failed: AP not STANDBY when standby == true from NOMINAL");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_6
  /* Requirement 6: AP MANEUVER -> STANDBY when standby == true and good == true (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 2.0; /* MANEUVER */
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay2_DSTATE = 1;   /* good = true */
  rtU.standby   = 1;
  rtU.supported = nondet_bool();
  rtU.apfail    = nondet_bool();
  rtU.limits    = nondet_bool();

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.STATE == 3.0, "Property 6 failed: AP not STANDBY when standby && good from MANEUVER");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_7
  /* Requirement 7: AP MANEUVER -> TRANSITION when supported == true and good == true (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 2.0; /* MANEUVER */
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay2_DSTATE = 1;   /* good = true */
  rtU.standby   = 0;
  rtU.supported = 1;
  rtU.apfail    = nondet_bool();
  rtU.limits    = nondet_bool();

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.STATE == 0.0, "Property 7 failed: AP not TRANSITION when supported && good from MANEUVER");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_8
  /* Requirement 8: AP STANDBY -> TRANSITION when standby == false (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 3.0; /* STANDBY */
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay2_DSTATE = 1;
  rtU.standby   = 0;
  rtU.supported = nondet_bool();
  rtU.apfail    = nondet_bool();
  rtU.limits    = nondet_bool();

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.STATE == 0.0, "Property 8 failed: AP not TRANSITION when standby == false from STANDBY");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_9
  /* Requirement 9: AP STANDBY -> MANEUVER when apfail == true (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 3.0; /* STANDBY */
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay2_DSTATE = 1;
  rtU.standby   = 1;
  rtU.supported = nondet_bool();
  rtU.apfail    = 1;
  rtU.limits    = nondet_bool();

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.STATE == 2.0, "Property 9 failed: AP not MANEUVER when apfail == true from STANDBY");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_10
  /* Requirement 10: Sensor NOMINAL -> FAULT when limits == true (immediate) */
  fsm_12B_global_initialize();
  rtDW.UnitDelay_DSTATE  = 1.0; /* AP NOMINAL to have sane outputs; not strictly required */
  rtDW.UnitDelay1_DSTATE = 0.0; /* Sensor NOMINAL */
  rtDW.UnitDelay2_DSTATE = 1;
  rtU.standby   = 0;
  rtU.supported = 1;
  rtU.apfail    = 0;
  rtU.limits    = 1; /* Exceed limits */

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.SENSTATE == 2.0, "Property 10 failed: Sensor not FAULT when limits == true from NOMINAL");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_11
  /* Requirement 11: Sensor NOMINAL -> TRANSITION when request == false (immediate) */
  fsm_12B_global_initialize();
  /* Force AP to STANDBY this cycle so request == false */
  rtDW.UnitDelay_DSTATE  = 3.0; /* AP STANDBY */
  rtDW.UnitDelay1_DSTATE = 0.0; /* Sensor NOMINAL */
  rtDW.UnitDelay2_DSTATE = 1;
  rtU.standby   = 1; /* keep AP in STANDBY */
  rtU.supported = nondet_bool();
  rtU.apfail    = 0;
  rtU.limits    = 0;

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.SENSTATE == 1.0, "Property 11 failed: Sensor not TRANSITION when request == false from NOMINAL");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_12
  /* Requirement 12: Sensor FAULT -> TRANSITION when request == false and limits == false (immediate) */
  fsm_12B_global_initialize();
  /* Force AP to STANDBY this cycle so request == false */
  rtDW.UnitDelay_DSTATE  = 3.0; /* AP STANDBY */
  rtDW.UnitDelay1_DSTATE = 2.0; /* Sensor FAULT */
  rtDW.UnitDelay2_DSTATE = 0;   /* good = false (consistent with FAULT) */
  rtU.standby   = 1;  /* keep STANDBY => request == false */
  rtU.supported = nondet_bool();
  rtU.apfail    = 0;
  rtU.limits    = 0;  /* limits not exceeded */

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.SENSTATE == 1.0, "Property 12 failed: Sensor not TRANSITION when request == false and limits == false from FAULT");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_13
  /* Requirement 13: Sensor TRANSITION -> NOMINAL when mode == true and request == true (immediate) */
  fsm_12B_global_initialize();
  /* Force AP to NOMINAL this cycle so mode == true and request == true */
  rtDW.UnitDelay_DSTATE  = 1.0; /* AP NOMINAL */
  rtDW.UnitDelay1_DSTATE = 1.0; /* Sensor TRANSITION */
  rtDW.UnitDelay2_DSTATE = 1;
  rtU.standby   = 0;  /* keep AP in NOMINAL */
  rtU.supported = 1;
  rtU.apfail    = 0;
  rtU.limits    = 0;

  max_steps = 1;
  i = 0;
  while (i < max_steps) {
    fsm_12B_global_step();
    __ESBMC_assert(rtY.SENSTATE == 0.0, "Property 13 failed: Sensor not NOMINAL when mode && request from TRANSITION");
    i++;
  }
#endif

#ifdef VERIFY_PROPERTY_14
  /* Additional check per instruction 7: verify "previous state" is kept via UnitDelays across cycles */
  fsm_12B_global_initialize();
  /* Start at AP TRANSITION, Sensor NOMINAL, good = true */
  rtDW.UnitDelay_DSTATE  = 0.0;
  rtDW.UnitDelay1_DSTATE = 0.0;
  rtDW.UnitDelay2_DSTATE = 1;

  /* Drive with nondet inputs for several cycles; only check UnitDelay consistency */
  max_steps = 5;
  exp_next_AP_state  = rtDW.UnitDelay_DSTATE;
  exp_next_SEN_state = rtDW.UnitDelay1_DSTATE;
  exp_next_good      = rtDW.UnitDelay2_DSTATE;

  i = 0;
  while (i < max_steps) {
    rtU.standby   = nondet_bool();
    rtU.supported = nondet_bool();
    rtU.apfail    = nondet_bool();
    rtU.limits    = nondet_bool();

    fsm_12B_global_step();

    if (i > 0) {
      __ESBMC_assert(rtDW.UnitDelay_DSTATE  == exp_next_AP_state,  "Property 14 failed: AP UnitDelay should equal previous cycle Merge");
      __ESBMC_assert(rtDW.UnitDelay1_DSTATE == exp_next_SEN_state, "Property 14 failed: SEN UnitDelay should equal previous cycle Merge_g");
      __ESBMC_assert(rtDW.UnitDelay2_DSTATE == exp_next_good,      "Property 14 failed: GOOD UnitDelay should equal !(prev Merge_g == 2.0)");
    }

    exp_next_AP_state  = rtDW.Merge;
    exp_next_SEN_state = rtDW.Merge_g;
    exp_next_good      = (rtDW.Merge_g != 2.0);
    i++;
  }
#endif

  return 0;
}