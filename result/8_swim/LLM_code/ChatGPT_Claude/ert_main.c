#include "swim_12B.h"
#include "swim_12B.c"
#include <math.h>
#include <assert.h>

// ESBMC assertion macro
#define __ESBMC_assert(cond) assert(cond)

// Non-deterministic functions for ESBMC
_Bool nondet_bool(void);
real_T nondet_real_T(void);
int nondet_int(void);

// Helper function to check floating point equality with tolerance
_Bool fp_equal(real_T a, real_T b, real_T tolerance) {
    return (a - b <= tolerance) && (b - a <= tolerance);
}

int main(void) {
    // Initialize the model
    swim_12B_initialize();
    
    // Variables to store previous values
    real_T prev_UnitDelay_DSTATE = 0.0;
    real_T prev_UnitDelay_DSTATE_d = 0.0;
    real_T prev_SWIM_ASWarningAllowed = 0.0;
    real_T prev_swimGet_AgcasLowSpeedWarn = 0.0;
    
    // Set reasonable loop number for verification
    int loop_count = 0;
    const int MAX_LOOPS = 5;
    
    // Main simulation loop
    while (loop_count < MAX_LOOPS) {
        // Store previous state values before step
        prev_UnitDelay_DSTATE = rtDW.UnitDelay_DSTATE;
        prev_UnitDelay_DSTATE_d = rtDW.UnitDelay_DSTATE_d;
        
        // Set non-deterministic inputs
        rtU.diGet_S_CatSwitchPosition = nondet_int();
        rtU.muxGet_T_Mux_AircraftGrossWeigh = nondet_real_T();
        rtU.diGet_S_LandingGearAltFlap = nondet_bool();
        rtU.E_AI_IMPACT_PRESSURE = nondet_real_T();
        rtU.muxGet_T_Mux_AirspeedMonitorEna = nondet_bool();
        rtU.cvGet_V_AgcasLowSpeedWarn = nondet_bool();
        rtU.olcGet_AgcasInterlocks = nondet_bool();
        
        // Constrain inputs to reasonable ranges
        if (rtU.muxGet_T_Mux_AircraftGrossWeigh < 0.0) {
            rtU.muxGet_T_Mux_AircraftGrossWeigh = 0.0;
        }
        if (rtU.muxGet_T_Mux_AircraftGrossWeigh > 100000.0) {
            rtU.muxGet_T_Mux_AircraftGrossWeigh = 100000.0;
        }
        if (rtU.E_AI_IMPACT_PRESSURE < 0.0) {
            rtU.E_AI_IMPACT_PRESSURE = 0.0;
        }
        if (rtU.E_AI_IMPACT_PRESSURE > 10000.0) {
            rtU.E_AI_IMPACT_PRESSURE = 10000.0;
        }
        
        // Store outputs before step for comparison
        if (loop_count > 0) {
            prev_SWIM_ASWarningAllowed = rtY.SWIM_ASWarningAllowed;
            prev_swimGet_AgcasLowSpeedWarn = rtY.swimGet_AgcasLowSpeedWarn;
        }
        
        // Execute model step
        swim_12B_step();
        
        #ifdef VERIFY_PROPERTY_1
        // Requirement 1: Auto GCAS Minimum Vcas computation
        {
            real_T expected_vmin;
            real_T expected_qcmin;
            real_T expected_qcmin_disable;
            
            // Calculate expected minimum airspeed
            if (rtU.diGet_S_CatSwitchPosition == 0) {
                expected_vmin = 1.25921 * sqrt(rtU.muxGet_T_Mux_AircraftGrossWeigh) + 10.0;
            } else {
                expected_vmin = 1.33694 * sqrt(rtU.muxGet_T_Mux_AircraftGrossWeigh) + 10.0;
            }
            
            // Calculate expected impact pressure thresholds
            expected_qcmin = ((0.020306 * expected_vmin - 2.0906) + 0.1) * 70.7184;
            expected_qcmin_disable = expected_qcmin - 28.72;
            
            // Verify outputs match expected values (with tolerance for floating point)
            __ESBMC_assert(fp_equal(rtY.SWIM_CalAirspeedmin_kts, expected_vmin, 0.001));
            __ESBMC_assert(fp_equal(rtY.SWIM_Qcmin_lbspft2, expected_qcmin, 0.001));
            __ESBMC_assert(fp_equal(rtY.swimGet_QcMinDisable_lbspft2, expected_qcmin_disable, 0.001));
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_2A
        // Requirement 2A: SWIM_ASWarningAllowed state transitions
        {
            real_T expected_warning_allowed;
            
            if (!rtU.diGet_S_LandingGearAltFlap) {
                // Should force to 0.0
                expected_warning_allowed = 0.0;
            } else if (rtU.diGet_S_LandingGearAltFlap && 
                      (rtU.E_AI_IMPACT_PRESSURE > rtY.SWIM_Qcmin_lbspft2)) {
                // Should force to 1.0
                expected_warning_allowed = 1.0;
            } else {
                // Should hold previous value
                expected_warning_allowed = prev_UnitDelay_DSTATE;
            }
            
            __ESBMC_assert(fp_equal(rtY.SWIM_ASWarningAllowed, expected_warning_allowed, 0.001));
            
            // Verify state will be updated correctly after this cycle
            __ESBMC_assert(fp_equal(rtDW.UnitDelay_DSTATE, rtY.SWIM_ASWarningAllowed, 0.001));
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_2B
        // Requirement 2B: Low speed warning generation when allowed
        {
            real_T expected_low_speed_warn;
            _Bool gating_enabled;
            
            // Check if gating is enabled
            gating_enabled = (rtU.muxGet_T_Mux_AirspeedMonitorEna || rtU.cvGet_V_AgcasLowSpeedWarn) &&
                            (!rtU.olcGet_AgcasInterlocks) &&
                            (rtY.SWIM_ASWarningAllowed == 1.0);
            
            if (gating_enabled) {
                // Warning should be based on current impact pressure comparison
                expected_low_speed_warn = (rtU.E_AI_IMPACT_PRESSURE < (rtY.SWIM_Qcmin_lbspft2 - 28.72)) ? 1.0 : 0.0;
            } else {
                // Should hold previous value
                expected_low_speed_warn = prev_UnitDelay_DSTATE_d;
            }
            
            __ESBMC_assert(fp_equal(rtY.swimGet_AgcasLowSpeedWarn, expected_low_speed_warn, 0.001));
            
            // Verify state will be updated correctly after this cycle
            __ESBMC_assert(fp_equal(rtDW.UnitDelay_DSTATE_d, rtY.swimGet_AgcasLowSpeedWarn, 0.001));
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3
        // Requirement 3: State persistence and update timing
        {
            // Verify that states are properly updated after swim_12B_step
            // The current rtDW values should match the outputs that were just computed
            __ESBMC_assert(fp_equal(rtDW.UnitDelay_DSTATE, rtY.SWIM_ASWarningAllowed, 0.001));
            __ESBMC_assert(fp_equal(rtDW.UnitDelay_DSTATE_d, rtY.swimGet_AgcasLowSpeedWarn, 0.001));
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_4
        // Requirement 4: Warning consistency checks
        {
            // If warning is not allowed, low speed warning should not be updated based on current conditions
            if (rtY.SWIM_ASWarningAllowed != 1.0) {
                _Bool gating_could_enable = (rtU.muxGet_T_Mux_AirspeedMonitorEna || rtU.cvGet_V_AgcasLowSpeedWarn) &&
                                           (!rtU.olcGet_AgcasInterlocks);
                if (gating_could_enable) {
                    // Even if other conditions are met, warning should hold previous value when not allowed
                    __ESBMC_assert(fp_equal(rtY.swimGet_AgcasLowSpeedWarn, prev_UnitDelay_DSTATE_d, 0.001));
                }
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_5
        // Requirement 5: Landing gear interlock
        {
            // When landing gear/flap is not deployed, warning should never be allowed
            if (!rtU.diGet_S_LandingGearAltFlap) {
                __ESBMC_assert(rtY.SWIM_ASWarningAllowed == 0.0);
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_6
        // Requirement 6: Impact pressure threshold hysteresis
        {
            // Verify the 28.72 lbspft2 hysteresis between enable and disable thresholds
            real_T enable_threshold = rtY.SWIM_Qcmin_lbspft2;
            real_T disable_threshold = rtY.swimGet_QcMinDisable_lbspft2;
            
            __ESBMC_assert(fp_equal(enable_threshold - disable_threshold, 28.72, 0.001));
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_7
        // Requirement 7: CAT switch effect on minimum airspeed
        {
            real_T vmin_cat0, vmin_cat_other;
            
            // Calculate both possible values
            vmin_cat0 = 1.25921 * sqrt(rtU.muxGet_T_Mux_AircraftGrossWeigh) + 10.0;
            vmin_cat_other = 1.33694 * sqrt(rtU.muxGet_T_Mux_AircraftGrossWeigh) + 10.0;
            
            if (rtU.diGet_S_CatSwitchPosition == 0) {
                __ESBMC_assert(fp_equal(rtY.SWIM_CalAirspeedmin_kts, vmin_cat0, 0.001));
            } else {
                __ESBMC_assert(fp_equal(rtY.SWIM_CalAirspeedmin_kts, vmin_cat_other, 0.001));
            }
            
            // CAT position 0 should always result in lower minimum airspeed
            __ESBMC_assert(vmin_cat0 <= vmin_cat_other);
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_8
        // Requirement 8: State initialization and bounds
        {
            // States should only be 0.0 or 1.0 (as they represent boolean conditions)
            __ESBMC_assert(rtDW.UnitDelay_DSTATE == 0.0 || rtDW.UnitDelay_DSTATE == 1.0);
            __ESBMC_assert(rtDW.UnitDelay_DSTATE_d == 0.0 || rtDW.UnitDelay_DSTATE_d == 1.0);
            __ESBMC_assert(rtY.SWIM_ASWarningAllowed == 0.0 || rtY.SWIM_ASWarningAllowed == 1.0);
            __ESBMC_assert(rtY.swimGet_AgcasLowSpeedWarn == 0.0 || rtY.swimGet_AgcasLowSpeedWarn == 1.0);
        }
        #endif
        
        loop_count++;
    }
    
    return 0;
}
