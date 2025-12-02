#include "integrator_12B.h"
#include "rtwtypes.h"
#include <math.h>

// ESBMC assertion macro
void __ESBMC_assert(_Bool cond, const char *msg);

// Nondet functions for ESBMC
_Bool nondet_bool(void);
real_T nondet_real(void);

// Helper function for floating point comparison
_Bool approx_equal(real_T a, real_T b, real_T tolerance) {
    return fabs(a - b) <= tolerance;
}

int main(void) {
    // Initialize the model
    integrator_12B_initialize();
    
    // Variables to store previous values for verification
    real_T prev_cmd;
    real_T prev_yout;
    real_T computed_tustin;
    
    // Loop counter
    int cycle = 0;
    const int MAX_CYCLES = 101; // For requirements 4a and 4b we need 101 cycles
    
    // Arrays for requirement 4b - cosine input sequence
    real_T cos_sequence[101] = {
        1.0, 0.995, 0.98, 0.955, 0.921, 0.878, 0.825, 0.764, 0.696, 0.621,
        0.54, 0.454, 0.362, 0.268, 0.17, 0.0707, -0.0292, -0.129, -0.227, -0.324,
        -0.416, -0.505, -0.588, -0.665, -0.735, -0.796, -0.849, -0.892, -0.925, -0.947,
        -0.959, -0.961, -0.951, -0.932, -0.902, -0.862, -0.813, -0.755, -0.689, -0.615,
        -0.535, -0.449, -0.358, -0.263, -0.166, -0.0664, 0.0335, 0.133, 0.232, 0.328,
        0.421, 0.509, 0.592, 0.669, 0.738, 0.799, 0.851, 0.894, 0.926, 0.948,
        0.96, 0.961, 0.951, 0.931, 0.9, 0.86, 0.81, 0.752, 0.685, 0.611,
        0.531, 0.444, 0.353, 0.259, 0.161, 0.062, -0.0378, -0.138, -0.236, -0.332,
        -0.425, -0.513, -0.596, -0.672, -0.741, -0.801, -0.853, -0.895, -0.928, -0.949,
        -0.96, -0.961, -0.95, -0.929, -0.898, -0.857, -0.807, -0.748, -0.681, -0.607,
        -0.526
    };
    
    // Expected sin output for requirement 4b
    real_T sin_expected[101] = {
        0.0, 0.0998, 0.198, 0.296, 0.389, 0.479, 0.565, 0.644, 0.717, 0.783,
        0.841, 0.891, 0.932, 0.964, 0.985, 0.997, 1.0, 0.991, 0.973, 0.946,
        0.909, 0.863, 0.808, 0.746, 0.676, 0.599, 0.516, 0.428, 0.335, 0.239,
        0.141, 0.0415, -0.0584, -0.158, -0.256, -0.351, -0.443, -0.53, -0.611, -0.686,
        -0.754, -0.813, -0.864, -0.906, -0.938, -0.96, -0.972, -0.974, -0.966, -0.947,
        -0.919, -0.881, -0.833, -0.778, -0.714, -0.644, -0.566, -0.483, -0.395, -0.302,
        -0.207, -0.11, -0.0124, 0.0854, 0.182, 0.276, 0.368, 0.456, 0.538, 0.615,
        0.685, 0.747, 0.801, 0.847, 0.883, 0.91, 0.927, 0.934, 0.931, 0.918,
        0.895, 0.862, 0.82, 0.77, 0.711, 0.645, 0.572, 0.493, 0.409, 0.32,
        0.228, 0.133, 0.0374, -0.0584, -0.154, -0.247, -0.338, -0.425, -0.507, -0.584,
        -0.654
    };
    
    while (cycle < MAX_CYCLES) {
        // Store previous values before step
        prev_cmd = rtDW.UnitDelay_DSTATE;
        prev_yout = rtDW.UnitDelay1_DSTATE;
        
        #ifdef VERIFY_PROPERTY_1A
        // Requirement 1a: When reset is true and BL <= ic <= TL, yout shall equal ic
        rtU.reset = 1;
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.ic = 5.0;  // Within bounds
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        
        integrator_12B_step();
        
        __ESBMC_assert(rtY.yout == rtU.ic, "Property 1a: yout should equal ic when reset and ic within bounds");
        #endif
        
        #ifdef VERIFY_PROPERTY_1B
        // Requirement 1b: When reset is true and TL >= BL and ic >= TL, yout shall equal TL
        rtU.reset = 1;
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.ic = 15.0;  // Above TL
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        
        integrator_12B_step();
        
        __ESBMC_assert(rtY.yout == rtU.TL, "Property 1b: yout should equal TL when reset and ic >= TL");
        #endif
        
        #ifdef VERIFY_PROPERTY_1C
        // Requirement 1c: When reset is true and TL >= BL and ic <= BL, yout shall equal BL
        rtU.reset = 1;
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.ic = -15.0;  // Below BL
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        
        integrator_12B_step();
        
        __ESBMC_assert(rtY.yout == rtU.BL, "Property 1c: yout should equal BL when reset and ic <= BL");
        #endif
        
        #ifdef VERIFY_PROPERTY_1D
        // Requirement 1d: Off-nominal case with TL < BL
        rtU.reset = 1;
        rtU.TL = -10.0;
        rtU.BL = 10.0;  // TL < BL (swapped)
        rtU.ic = 0.0;   // Between TL and BL
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        
        integrator_12B_step();
        
        // With swapped limits, ic is within bounds, so yout should equal ic
        __ESBMC_assert(rtY.yout == rtU.ic, "Property 1d: yout should equal ic when reset and TL<BL with ic in range");
        #endif
        
        #ifdef VERIFY_PROPERTY_1E
        // Requirement 1e: Off-nominal case with TL < BL and saturation
        rtU.reset = 1;
        rtU.TL = -10.0;
        rtU.BL = 10.0;  // TL < BL (swapped)
        rtU.ic = 15.0;  // Above BL (which is the upper limit when swapped)
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        
        integrator_12B_step();
        
        // With swapped limits, BL is the upper bound
        __ESBMC_assert(rtY.yout == rtU.BL, "Property 1e: yout should equal BL when reset and TL<BL with ic > BL");
        #endif
        
        #ifdef VERIFY_PROPERTY_2A
        // Requirement 2a: If TL >= BL, yout shall satisfy BL <= yout <= TL
        rtU.reset = nondet_bool();
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.ic = nondet_real();
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        
        integrator_12B_step();
        
        __ESBMC_assert(rtY.yout >= rtU.BL && rtY.yout <= rtU.TL, 
                      "Property 2a: yout should be within [BL, TL] when TL >= BL");
        #endif
        
        #ifdef VERIFY_PROPERTY_2B
        // Requirement 2b: If TL < BL (off-nominal), yout shall satisfy TL <= yout <= BL
        rtU.reset = nondet_bool();
        rtU.TL = -10.0;
        rtU.BL = 10.0;  // TL < BL
        rtU.ic = nondet_real();
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        
        integrator_12B_step();
        
        __ESBMC_assert(rtY.yout >= rtU.TL && rtY.yout <= rtU.BL, 
                      "Property 2b: yout should be within [TL, BL] when TL < BL");
        #endif
        
        #ifdef VERIFY_PROPERTY_3_1
        // Requirement 3-1: Specific case with constraints
        rtU.reset = 0;
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.cmd = 5.0;  // |cmd| < 10
        rtU.T = 0.1;
        rtU.ic = 0.0;
        
        // Compute expected Tustin formula result
        computed_tustin = 0.5 * rtU.T * (rtU.cmd + prev_cmd) + prev_yout;
        
        integrator_12B_step();
        
        // Only verify if result is within bounds (no saturation)
        if (computed_tustin >= rtU.BL && computed_tustin <= rtU.TL) {
            __ESBMC_assert(approx_equal(rtY.yout, computed_tustin, 0.0001),
                          "Property 3-1: yout should equal Tustin formula when not saturated");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3_2
        // Requirement 3-2: General normal operation, unclamped
        rtU.reset = 0;
        rtU.TL = 100.0;
        rtU.BL = -100.0;
        rtU.cmd = nondet_real();
        rtU.T = 0.1;
        rtU.ic = 0.0;
        
        // Compute expected Tustin formula result
        computed_tustin = 0.5 * rtU.T * (rtU.cmd + prev_cmd) + prev_yout;
        
        integrator_12B_step();
        
        // Verify if within bounds
        if (computed_tustin >= rtU.BL && computed_tustin <= rtU.TL) {
            __ESBMC_assert(approx_equal(rtY.yout, computed_tustin, 0.0001),
                          "Property 3-2: yout should equal Tustin formula when within bounds");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3_3
        // Requirement 3-3: Normal operation with saturation
        rtU.reset = 0;
        rtU.TL = 10.0;
        rtU.BL = -10.0;
        rtU.cmd = 100.0;  // Large value to force saturation
        rtU.T = 0.1;
        rtU.ic = 0.0;
        
        // Compute expected Tustin formula result
        computed_tustin = 0.5 * rtU.T * (rtU.cmd + prev_cmd) + prev_yout;
        
        integrator_12B_step();
        
        // Verify saturation behavior
        if (computed_tustin > rtU.TL) {
            __ESBMC_assert(rtY.yout == rtU.TL, "Property 3-3: yout should saturate to TL");
        } else if (computed_tustin < rtU.BL) {
            __ESBMC_assert(rtY.yout == rtU.BL, "Property 3-3: yout should saturate to BL");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_4A
        // Requirement 4a: Integration accuracy with constant input
        rtU.reset = 0;
        rtU.TL = 100.0;  // Large bounds to avoid saturation
        rtU.BL = -100.0;
        rtU.cmd = 1.0;
        rtU.T = 0.1;
        rtU.ic = 0.0;
        
        integrator_12B_step();
        
        // Expected value: k * T where k is the cycle number
        real_T expected = cycle * rtU.T;
        __ESBMC_assert(approx_equal(rtY.yout, expected, 0.1),
                      "Property 4a: Integration of constant 1.0 should yield k*T");
        #endif
        
        #ifdef VERIFY_PROPERTY_4B
        // Requirement 4b: Integration accuracy with cosine input
        if (cycle < 101) {
            rtU.reset = 0;
            rtU.TL = 100.0;  // Large bounds to avoid saturation
            rtU.BL = -100.0;
            rtU.cmd = cos_sequence[cycle];
            rtU.T = 0.1;
            rtU.ic = 0.0;
            
            integrator_12B_step();
            
            __ESBMC_assert(approx_equal(rtY.yout, sin_expected[cycle], 0.1),
                          "Property 4b: Integration of cos(t) should yield sin(t)");
        }
        #endif
        
        cycle++;
    }
    
    return 0;
}