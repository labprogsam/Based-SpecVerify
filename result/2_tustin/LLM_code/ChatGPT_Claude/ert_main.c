#include "integrator_12B.h"
#include <math.h>
#include <stdio.h>

// ESBMC assertion macro
void __ESBMC_assert(_Bool cond, const char* msg);

// Non-deterministic functions for ESBMC
_Bool nondet_bool(void);
real_T nondet_real(void);

// Helper function for floating point comparison
_Bool approx_equal(real_T a, real_T b, real_T tol) {
    return fabs(a - b) <= tol;
}

int main(void) {
    // Initialize the model
    integrator_12B_initialize();
    
    // Variables to track previous values for verification
    real_T prev_cmd;
    real_T prev_yout;
    real_T pre_limit_value;
    
    // Loop counter
    int cycle = 0;
    const int MAX_CYCLES = 101; // For requirement 4 tests
    
    // Arrays for requirement 4 tests
    real_T cos_values[101];
    real_T sin_expected[101];
    
    // Initialize cosine input and expected sine output for Req 4b
    for (int i = 0; i <= 100; i++) {
        real_T t = i * 0.1;
        cos_values[i] = cos(t);
        sin_expected[i] = sin(t);
    }
    
    // Main verification loop
    while (cycle < MAX_CYCLES) {
        // Store previous values before step
        prev_cmd = (cycle > 0) ? rtU.cmd : 0.0;
        prev_yout = (cycle > 0) ? rtY.yout : 0.0;
        
        // Set inputs (can be non-deterministic or specific for properties)
        #ifdef VERIFY_PROPERTY_1A
        if (cycle == 0) {
            rtU.reset = 1;
            rtU.TL = 10.0;
            rtU.BL = -10.0;
            rtU.ic = 5.0; // Within bounds
            rtU.T = 0.1;
            rtU.cmd = nondet_real();
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1B
        if (cycle == 0) {
            rtU.reset = 1;
            rtU.TL = 10.0;
            rtU.BL = -10.0;
            rtU.ic = 15.0; // Above TL
            rtU.T = 0.1;
            rtU.cmd = nondet_real();
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1C
        if (cycle == 0) {
            rtU.reset = 1;
            rtU.TL = 10.0;
            rtU.BL = -10.0;
            rtU.ic = -15.0; // Below BL
            rtU.T = 0.1;
            rtU.cmd = nondet_real();
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1D
        if (cycle == 0) {
            rtU.reset = 1;
            rtU.TL = -10.0; // TL < BL (off-nominal)
            rtU.BL = 10.0;
            rtU.ic = 0.0; // Between TL and BL
            rtU.T = 0.1;
            rtU.cmd = nondet_real();
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1E
        if (cycle == 0) {
            rtU.reset = 1;
            rtU.TL = -10.0; // TL < BL (off-nominal)
            rtU.BL = 10.0;
            rtU.ic = -15.0; // Below TL
            rtU.T = 0.1;
            rtU.cmd = nondet_real();
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_2A
        rtU.reset = 0;
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.T = 0.1;
        rtU.ic = 0.0;
        rtU.cmd = nondet_real();
        #endif
        
        #ifdef VERIFY_PROPERTY_2B
        rtU.reset = 0;
        rtU.TL = -10.0; // TL < BL (off-nominal)
        rtU.BL = 10.0;
        rtU.T = 0.1;
        rtU.ic = 0.0;
        rtU.cmd = nondet_real();
        #endif
        
        #ifdef VERIFY_PROPERTY_3_1
        rtU.reset = 0;
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.T = 0.1;
        rtU.ic = 0.0;
        rtU.cmd = nondet_real();
        // Constrain cmd to be within bounds
        if (rtU.cmd > 9.9) rtU.cmd = 9.9;
        if (rtU.cmd < -9.9) rtU.cmd = -9.9;
        #endif
        
        #ifdef VERIFY_PROPERTY_3_2
        rtU.reset = 0;
        rtU.TL = nondet_real();
        rtU.BL = nondet_real();
        rtU.T = 0.1;
        rtU.ic = 0.0;
        rtU.cmd = nondet_real();
        // Ensure TL >= BL for normal operation
        if (rtU.TL < rtU.BL) {
            real_T temp = rtU.TL;
            rtU.TL = rtU.BL;
            rtU.BL = temp;
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3_3
        rtU.reset = 0;
        rtU.TL = nondet_real();
        rtU.BL = nondet_real();
        rtU.T = 0.1;
        rtU.ic = 0.0;
        rtU.cmd = nondet_real();
        #endif
        
        #ifdef VERIFY_PROPERTY_4A
        rtU.reset = 0;
        rtU.TL = 100.0; // Large bounds to avoid saturation
        rtU.BL = -100.0;
        rtU.T = 0.1;
        rtU.ic = 0.0;
        rtU.cmd = 1.0; // Constant input
        #endif
        
        #ifdef VERIFY_PROPERTY_4B
        rtU.reset = 0;
        rtU.TL = 2.0; // Bounds sufficient for sine wave
        rtU.BL = -2.0;
        rtU.T = 0.1;
        rtU.ic = 0.0;
        rtU.cmd = cos_values[cycle]; // Cosine input sequence
        #endif
        
        // Execute the model step
        integrator_12B_step();
        
        // Calculate pre-limit value for verification
        if (rtU.reset) {
            pre_limit_value = rtU.ic;
        } else if (cycle > 0) {
            pre_limit_value = 0.5 * rtU.T * (rtU.cmd + prev_cmd) + prev_yout;
        } else {
            pre_limit_value = 0.5 * rtU.T * rtU.cmd;
        }
        
        // Property verification after step execution
        
        #ifdef VERIFY_PROPERTY_1A
        // Req 1a: When reset is true and BL <= ic <= TL, yout shall equal ic
        if (cycle == 0) {
            __ESBMC_assert(rtY.yout == rtU.ic, 
                "Property 1a: yout should equal ic when reset and ic within bounds");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1B
        // Req 1b: When reset is true and TL >= BL and ic >= TL, yout shall equal TL
        if (cycle == 0) {
            __ESBMC_assert(rtY.yout == rtU.TL, 
                "Property 1b: yout should equal TL when reset and ic >= TL");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1C
        // Req 1c: When reset is true and TL >= BL and ic <= BL, yout shall equal BL
        if (cycle == 0) {
            __ESBMC_assert(rtY.yout == rtU.BL, 
                "Property 1c: yout should equal BL when reset and ic <= BL");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1D
        // Req 1d: When reset is true, TL < BL, and ic within swapped bounds
        if (cycle == 0) {
            // With TL < BL, effective bounds are [TL, BL] with TL as lower, BL as upper
            if (rtU.ic >= rtU.BL) {
                __ESBMC_assert(rtY.yout == rtU.BL, 
                    "Property 1d: yout should equal BL when ic >= BL (off-nominal)");
            } else if (rtU.ic <= rtU.TL) {
                __ESBMC_assert(rtY.yout == rtU.TL, 
                    "Property 1d: yout should equal TL when ic <= TL (off-nominal)");
            } else {
                __ESBMC_assert(rtY.yout == rtU.ic, 
                    "Property 1d: yout should equal ic when TL < ic < BL (off-nominal)");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_1E
        // Req 1e: Same as 1d for off-nominal case
        if (cycle == 0) {
            if (rtU.ic <= rtU.TL) {
                __ESBMC_assert(rtY.yout == rtU.TL, 
                    "Property 1e: yout should equal TL when ic <= TL (off-nominal)");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_2A
        // Req 2a: If TL >= BL, yout shall satisfy BL <= yout <= TL
        if (rtU.TL >= rtU.BL) {
            __ESBMC_assert(rtY.yout >= rtU.BL && rtY.yout <= rtU.TL, 
                "Property 2a: yout should be within [BL, TL] when TL >= BL");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_2B
        // Req 2b: If TL < BL (off-nominal), yout shall satisfy TL <= yout <= BL
        if (rtU.TL < rtU.BL) {
            __ESBMC_assert(rtY.yout >= rtU.TL && rtY.yout <= rtU.BL, 
                "Property 2b: yout should be within [TL, BL] when TL < BL");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3_1
        // Req 3-1: Specific case with exact parameters
        if (cycle > 0 && fabs(rtU.cmd) < 10.0) {
            real_T expected = 0.5 * rtU.T * (rtU.cmd + prev_cmd) + prev_yout;
            if (expected >= -10.0 && expected <= 10.0) {
                __ESBMC_assert(approx_equal(rtY.yout, expected, 1e-9), 
                    "Property 3-1: yout should match Tustin formula for specific case");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3_2
        // Req 3-2: General normal operation, unclamped
        if (cycle > 0 && !rtU.reset && rtU.TL >= rtU.BL) {
            real_T expected = 0.5 * rtU.T * (rtU.cmd + prev_cmd) + prev_yout;
            if (expected >= rtU.BL && expected <= rtU.TL) {
                __ESBMC_assert(approx_equal(rtY.yout, expected, 1e-9), 
                    "Property 3-2: yout should match Tustin formula when within bounds");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3_3
        // Req 3-3: Normal operation with saturation
        if (cycle > 0 && !rtU.reset) {
            real_T expected = 0.5 * rtU.T * (rtU.cmd + prev_cmd) + prev_yout;
            if (rtU.TL >= rtU.BL) {
                if (expected > rtU.TL) {
                    __ESBMC_assert(rtY.yout == rtU.TL, 
                        "Property 3-3: yout should saturate to TL");
                } else if (expected < rtU.BL) {
                    __ESBMC_assert(rtY.yout == rtU.BL, 
                        "Property 3-3: yout should saturate to BL");
                }
            } else {
                // Off-nominal: TL < BL
                if (expected > rtU.BL) {
                    __ESBMC_assert(rtY.yout == rtU.BL, 
                        "Property 3-3: yout should saturate to BL (off-nominal)");
                } else if (expected < rtU.TL) {
                    __ESBMC_assert(rtY.yout == rtU.TL, 
                        "Property 3-3: yout should saturate to TL (off-nominal)");
                }
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_4A
        // Req 4a: Integration accuracy with constant input
        real_T expected_4a = cycle * rtU.T;
        __ESBMC_assert(approx_equal(rtY.yout, expected_4a, 0.1), 
            "Property 4a: yout should approximate k*T for constant input");
        #endif
        
        #ifdef VERIFY_PROPERTY_4B
        // Req 4b: Integration accuracy with cosine input (sine output)
        __ESBMC_assert(approx_equal(rtY.yout, sin_expected[cycle], 0.1), 
            "Property 4b: yout should approximate sin(t) for cos(t) input");
        #endif
        
        cycle++;
    }
    
    return 0;
}
