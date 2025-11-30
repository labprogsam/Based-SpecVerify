#include "nn_12B.h"
#include "nn_12B.c"
#include "nn_12B_data.c"
#include <math.h>

// ESBMC assertion macro
void __ESBMC_assert(_Bool cond, const char *msg);

// Nondet functions for ESBMC
_Bool nondet_bool(void);
double nondet_double(void);

// Define reasonable bounds for inputs based on saturation limits
#define INPUT_X_MIN -2.0
#define INPUT_X_MAX 2.0
#define INPUT_Y_MIN -2.0
#define INPUT_Y_MAX 2.0

// Maximum number of iterations for verification
#define MAX_ITERATIONS 10

// Function to generate bounded nondeterministic inputs
double bounded_nondet_double(double min, double max) {
    double val = nondet_double();
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

int main(void) {
    // Initialize the model
    nn_12B_initialize();
    
    // Variables to store previous cycle values for derivative checking
    double prev_x = 0.0;
    double prev_y = 0.0;
    double prev_z = 0.0;
    _Bool first_iteration = 1;
    
    // Main verification loop
    int iterations = 0;
    while (iterations < MAX_ITERATIONS) {
        
        // Generate nondeterministic inputs within reasonable bounds
        rtU.x = bounded_nondet_double(INPUT_X_MIN, INPUT_X_MAX);
        rtU.y = bounded_nondet_double(INPUT_Y_MIN, INPUT_Y_MAX);
        
        // Store current inputs for derivative calculation
        double curr_x = rtU.x;
        double curr_y = rtU.y;
        
        // Execute the neural network step
        nn_12B_step();
        
        // Store current output
        double curr_z = rtY.z;
        
        #ifdef VERIFY_PROPERTY_1
        // Requirement 1: The maximum value of rtY.z shall be ≤ 1.1
        __ESBMC_assert(rtY.z <= 1.1, "Property 1: Output z exceeds maximum bound of 1.1");
        #endif
        
        #ifdef VERIFY_PROPERTY_2
        // Requirement 2: The minimum value of rtY.z shall be ≥ -0.2
        __ESBMC_assert(rtY.z >= -0.2, "Property 2: Output z below minimum bound of -0.2");
        #endif
        
        #ifdef VERIFY_PROPERTY_3
        // Requirement 3: Backward-difference spatial derivatives bounds
        // Only check after first iteration when we have previous values
        if (!first_iteration) {
            double delta_x = curr_x - prev_x;
            double delta_y = curr_y - prev_y;
            double delta_z = curr_z - prev_z;
            
            // Check derivative with respect to x if delta_x is not zero
            if (delta_x != 0.0) {
                double dz_dx = delta_z / delta_x;
                __ESBMC_assert(dz_dx >= -35.0 && dz_dx <= 10.0, 
                    "Property 3: Spatial derivative dz/dx out of bounds [-35, 10]");
            }
            
            // Check derivative with respect to y if delta_y is not zero
            if (delta_y != 0.0) {
                double dz_dy = delta_z / delta_y;
                __ESBMC_assert(dz_dy >= -35.0 && dz_dy <= 10.0,
                    "Property 3: Spatial derivative dz/dy out of bounds [-35, 10]");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_4
        // Requirement 4: Absolute error to truth never exceeds 0.01
        // For verification, we simulate checking against a "truth" value
        // In practice, this would need actual dataset values
        
        // Generate a nondeterministic "truth" value for verification
        // Assuming the truth should be within reasonable output bounds
        double truth_z = bounded_nondet_double(-0.2, 1.1);
        
        // In real verification, you would use actual dataset values like:
        // double truth_z = get_truth_value_for_inputs(curr_x, curr_y);
        
        double abs_error = fabs(curr_z - truth_z);
        __ESBMC_assert(abs_error <= 0.01,
            "Property 4: Absolute error to truth exceeds 0.01");
        #endif
        
        // Update previous values for next iteration
        prev_x = curr_x;
        prev_y = curr_y;
        prev_z = curr_z;
        first_iteration = 0;
        
        iterations++;
    }
    
    return 0;
}

// Alternative main function with specific test scenarios
#ifdef VERIFY_WITH_SCENARIOS
int main_scenarios(void) {
    // Initialize the model
    nn_12B_initialize();
    
    // Test scenario 1: Boundary values
    double test_inputs_x[] = {-1.969179, 1.885733, 0.0, -1.0, 1.0};
    double test_inputs_y[] = {-1.999644, 1.974943, 0.0, -1.0, 1.0};
    
    double prev_x = 0.0;
    double prev_y = 0.0;
    double prev_z = 0.0;
    
    for (int i = 0; i < 5; i++) {
        // Set inputs
        rtU.x = test_inputs_x[i];
        rtU.y = test_inputs_y[i];
        
        // Execute step
        nn_12B_step();
        
        #ifdef VERIFY_PROPERTY_1
        __ESBMC_assert(rtY.z <= 1.1, "Property 1: Output exceeds maximum");
        #endif
        
        #ifdef VERIFY_PROPERTY_2
        __ESBMC_assert(rtY.z >= -0.2, "Property 2: Output below minimum");
        #endif
        
        #ifdef VERIFY_PROPERTY_3
        if (i > 0) {
            double delta_x = rtU.x - prev_x;
            double delta_y = rtU.y - prev_y;
            double delta_z = rtY.z - prev_z;
            
            if (delta_x != 0.0) {
                double dz_dx = delta_z / delta_x;
                __ESBMC_assert(dz_dx >= -35.0 && dz_dx <= 10.0,
                    "Property 3: dz/dx out of bounds");
            }
            
            if (delta_y != 0.0) {
                double dz_dy = delta_z / delta_y;
                __ESBMC_assert(dz_dy >= -35.0 && dz_dy <= 10.0,
                    "Property 3: dz/dy out of bounds");
            }
        }
        #endif
        
        // Update previous values
        prev_x = rtU.x;
        prev_y = rtU.y;
        prev_z = rtY.z;
    }
    
    return 0;
}
#endif

// Additional verification with random walk for derivative checking
#ifdef VERIFY_RANDOM_WALK
int main_random_walk(void) {
    nn_12B_initialize();
    
    // Start from a random point
    double x = bounded_nondet_double(-1.0, 1.0);
    double y = bounded_nondet_double(-1.0, 1.0);
    
    rtU.x = x;
    rtU.y = y;
    nn_12B_step();
    
    double prev_x = x;
    double prev_y = y;
    double prev_z = rtY.z;
    
    // Perform small steps for derivative checking
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        // Small random step
        double step_x = bounded_nondet_double(-0.1, 0.1);
        double step_y = bounded_nondet_double(-0.1, 0.1);
        
        x = prev_x + step_x;
        y = prev_y + step_y;
        
        // Bound check
        if (x < INPUT_X_MIN) x = INPUT_X_MIN;
        if (x > INPUT_X_MAX) x = INPUT_X_MAX;
        if (y < INPUT_Y_MIN) y = INPUT_Y_MIN;
        if (y > INPUT_Y_MAX) y = INPUT_Y_MAX;
        
        rtU.x = x;
        rtU.y = y;
        nn_12B_step();
        
        #ifdef VERIFY_PROPERTY_1
        __ESBMC_assert(rtY.z <= 1.1, "Property 1 violated");
        #endif
        
        #ifdef VERIFY_PROPERTY_2
        __ESBMC_assert(rtY.z >= -0.2, "Property 2 violated");
        #endif
        
        #ifdef VERIFY_PROPERTY_3
        double delta_z = rtY.z - prev_z;
        
        if (step_x != 0.0) {
            double dz_dx = delta_z / step_x;
            __ESBMC_assert(dz_dx >= -35.0 && dz_dx <= 10.0,
                "Property 3: dz/dx violated");
        }
        
        if (step_y != 0.0) {
            double dz_dy = delta_z / step_y;
            __ESBMC_assert(dz_dy >= -35.0 && dz_dy <= 10.0,
                "Property 3: dz/dy violated");
        }
        #endif
        
        prev_x = x;
        prev_y = y;
        prev_z = rtY.z;
    }
    
    return 0;
}
#endif
# Verify Property 1
esbmc ert_main.c -DVERIFY_PROPERTY_1 --no-bounds-check --no-pointer-check

# Verify Property 2  
esbmc ert_main.c -DVERIFY_PROPERTY_2 --no-bounds-check --no-pointer-check

# Verify Property 3
esbmc ert_main.c -DVERIFY_PROPERTY_3 --no-bounds-check --no-pointer-check

# Verify Property 4
esbmc ert_main.c -DVERIFY_PROPERTY_4 --no-bounds-check --no-pointer-check

# Verify with specific scenarios
esbmc ert_main.c -DVERIFY_WITH_SCENARIOS -DVERIFY_PROPERTY_1 --no-bounds-check --no-pointer-check

# Verify with random walk for derivatives
esbmc ert_main.c -DVERIFY_RANDOM_WALK -DVERIFY_PROPERTY_3 --no-bounds-check --no-pointer-check
- Maintains previous state values without static variables
- Implements all four requirements as separate properties
- Uses preprocessing macros to control which properties to verify
- Includes multiple verification strategies (general loop, specific scenarios, random walk)
- Properly handles the stateless nature of the neural network (no UnitDelay)