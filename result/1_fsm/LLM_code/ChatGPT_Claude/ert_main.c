#include "fsm_12B_global.h"
#include "fsm_12B_global.c"
#include <stdbool.h>

// ESBMC nondet functions
_Bool nondet_bool();

// Helper function to save previous state values
void save_prev_state(real_T *prev_ap_state, real_T *prev_sen_state, _Bool *prev_sen_good) {
    *prev_ap_state = rtDW.UnitDelay_DSTATE;
    *prev_sen_state = rtDW.UnitDelay1_DSTATE;
    *prev_sen_good = rtDW.UnitDelay2_DSTATE;
}

int main() {
    // Variables to track previous states
    real_T prev_ap_state;
    real_T prev_sen_state;
    _Bool prev_sen_good;
    
    // Initialize the system
    fsm_12B_global_initialize();
    
    // Run system for multiple cycles
    int loop_count = 0;
    while (loop_count < 10) {
        // Save previous state before step
        save_prev_state(&prev_ap_state, &prev_sen_state, &prev_sen_good);
        
        // Set nondeterministic inputs
        rtU.standby = nondet_bool();
        rtU.apfail = nondet_bool();
        rtU.supported = nondet_bool();
        rtU.limits = nondet_bool();
        
        // Run one step
        fsm_12B_global_step();
        
#ifdef VERIFY_PROPERTY_1
        // Requirement 1: Exceeding sensor limits shall latch an autopilot pullup
        // Check at t1 after limits was true at t0
        if (loop_count >= 1) {
            // If at previous cycle: AP was NOMINAL, sensor was NOMINAL, 
            // limits became true, and conditions met
            if (prev_ap_state == 1.0 && prev_sen_state == 0.0 && 
                rtU.limits && !rtU.standby && rtU.supported && !rtU.apfail) {
                // After one more cycle, pullup should be active
                save_prev_state(&prev_ap_state, &prev_sen_state, &prev_sen_good);
                rtU.standby = false;
                rtU.supported = true;
                rtU.apfail = false;
                rtU.limits = true;
                fsm_12B_global_step();
                __ESBMC_assert(rtY.STATE == 2.0 && rtY.pullup == true,
                              "Property 1: Pullup should latch after limits exceeded");
            }
        }
#endif

#ifdef VERIFY_PROPERTY_2
        // Requirement 2: AP changes from TRANSITION to STANDBY when standby is true
        if (prev_ap_state == 0.0 && rtU.standby == true) {
            __ESBMC_assert(rtY.STATE == 3.0,
                          "Property 2: AP should transition from TRANSITION to STANDBY when standby");
        }
#endif

#ifdef VERIFY_PROPERTY_3
        // Requirement 3: AP changes from TRANSITION to NOMINAL when supported and sensor good
        if (prev_ap_state == 0.0 && prev_sen_good == true && rtU.supported == true) {
            __ESBMC_assert(rtY.STATE == 1.0,
                          "Property 3: AP should transition from TRANSITION to NOMINAL when supported and sensor good");
        }
#endif

#ifdef VERIFY_PROPERTY_4
        // Requirement 4: AP changes from NOMINAL to MANEUVER when sensor not good
        if (prev_ap_state == 1.0 && prev_sen_good == false) {
            __ESBMC_assert(rtY.STATE == 2.0 && rtY.pullup == true,
                          "Property 4: AP should transition from NOMINAL to MANEUVER when sensor not good");
        }
#endif

#ifdef VERIFY_PROPERTY_5
        // Requirement 5: AP changes from NOMINAL to STANDBY when standby
        if (prev_ap_state == 1.0 && rtU.standby == true) {
            __ESBMC_assert(rtY.STATE == 3.0,
                          "Property 5: AP should transition from NOMINAL to STANDBY when standby");
        }
#endif

#ifdef VERIFY_PROPERTY_6
        // Requirement 6: AP changes from MANEUVER to STANDBY when standby and sensor good
        if (prev_ap_state == 2.0 && prev_sen_good == true && rtU.standby == true) {
            __ESBMC_assert(rtY.STATE == 3.0 && rtY.pullup == false,
                          "Property 6: AP should transition from MANEUVER to STANDBY when standby and sensor good");
        }
#endif

#ifdef VERIFY_PROPERTY_7
        // Requirement 7: AP changes from MANEUVER to TRANSITION when supported and sensor good
        if (prev_ap_state == 2.0 && prev_sen_good == true && rtU.supported == true) {
            __ESBMC_assert(rtY.STATE == 0.0 && rtY.pullup == false,
                          "Property 7: AP should transition from MANEUVER to TRANSITION when supported and sensor good");
        }
#endif

#ifdef VERIFY_PROPERTY_8
        // Requirement 8: AP changes from STANDBY to TRANSITION when not standby
        if (prev_ap_state == 3.0 && rtU.standby == false) {
            __ESBMC_assert(rtY.STATE == 0.0,
                          "Property 8: AP should transition from STANDBY to TRANSITION when not standby");
        }
#endif

#ifdef VERIFY_PROPERTY_9
        // Requirement 9: AP changes from STANDBY to MANEUVER when apfail
        if (prev_ap_state == 3.0 && rtU.apfail == true) {
            __ESBMC_assert(rtY.STATE == 2.0 && rtY.pullup == true,
                          "Property 9: AP should transition from STANDBY to MANEUVER when apfail");
        }
#endif

#ifdef VERIFY_PROPERTY_10
        // Requirement 10: Sensor changes from NOMINAL to FAULT when limits exceeded
        if (prev_sen_state == 0.0 && rtU.limits == true) {
            __ESBMC_assert(rtY.SENSTATE == 2.0,
                          "Property 10: Sensor should transition from NOMINAL to FAULT when limits");
        }
#endif

#ifdef VERIFY_PROPERTY_11
        // Requirement 11: Sensor changes from NOMINAL to TRANSITION when not request
        // REQUEST is false only when AP is in STANDBY (3.0)
        if (prev_sen_state == 0.0 && rtDW.Merge == 3.0) {
            __ESBMC_assert(rtY.SENSTATE == 1.0,
                          "Property 11: Sensor should transition from NOMINAL to TRANSITION when not request");
        }
#endif

#ifdef VERIFY_PROPERTY_12
        // Requirement 12: Sensor changes from FAULT to TRANSITION when not request OR not limits
        if (prev_sen_state == 2.0) {
            // Check if transition should occur
            _Bool should_transition = (rtDW.Merge == 3.0) || (!rtU.limits);
            if (should_transition) {
                __ESBMC_assert(rtY.SENSTATE == 1.0,
                              "Property 12: Sensor should transition from FAULT to TRANSITION when not request or not limits");
            }
        }
#endif

#ifdef VERIFY_PROPERTY_13
        // Requirement 13: Sensor changes from TRANSITION to NOMINAL when request AND mode
        if (prev_sen_state == 1.0) {
            // REQUEST is true when AP not in STANDBY
            // MODE is true when AP not in TRANSITION
            _Bool request = (rtDW.Merge != 3.0);
            _Bool mode = (rtDW.Merge != 0.0);
            if (request && mode) {
                __ESBMC_assert(rtY.SENSTATE == 0.0,
                              "Property 13: Sensor should transition from TRANSITION to NOMINAL when request and mode");
            }
        }
#endif

        loop_count++;
    }
    
    return 0;
}
// 4. **Verifies each requirement separately** - Each property is wrapped in its own `#ifdef` block for selective verification

// 5. **Handles timing requirements correctly**:
//    - Property 1 handles the one-cycle delay for sensor fault affecting autopilot
//    - Other properties check immediate transitions based on current conditions

// 6. **Checks state consistency** - Verifies both state values and output signals (like pullup)

// 7. **Uses correct state encodings**:
//    - Autopilot: 0.0=TRANSITION, 1.0=NOMINAL, 2.0=MANEUVER, 3.0=STANDBY
//    - Sensor: 0.0=NOMINAL, 1.0=TRANSITION, 2.0=FAULT

// 8. **Properly handles internal signals** - For requirements involving REQUEST and MODE, the code correctly derives these from the autopilot state

// The code can be used with ESBMC by defining the appropriate `VERIFY_PROPERTY_N` macro during compilation to verify each requirement individually.