#include "EB_12B.h"
#include "EB_12B_data.c"
#include <math.h>
#include <string.h>

// Nondet functions for ESBMC
_Bool nondet_bool(void);
double nondet_double(void);

// Helper function to compute 2-norm of a 3x1 vector
double compute_norm2_3x1(const double v[3]) {
    return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

// Helper function to compute matrix determinant 3x3
double compute_det_3x3(const double M[9]) {
    return M[0] * (M[4] * M[8] - M[5] * M[7]) -
           M[1] * (M[3] * M[8] - M[5] * M[6]) +
           M[2] * (M[3] * M[7] - M[4] * M[6]);
}

int main(void) {
    // Initialize the system
    EB_12B_initialize();
    
    // Variables to track previous state (not using static)
    double prev_check[9];
    double prev_yinv[9];
    double prev_P_k[15];
    double prev_u[5];
    double prev_Buminusd[3];
    double prev_J;
    _Bool prev_ridge_on;
    
    // Set loop counter
    int loop_count = 10; // Reasonable number of loops
    int i = 0;
    
    while (i < loop_count) {
        // Save previous outputs
        if (i > 0) {
            memcpy(prev_check, rtY.check, 9 * sizeof(double));
            memcpy(prev_yinv, rtY.yinv, 9 * sizeof(double));
            memcpy(prev_P_k, rtY.P_k, 15 * sizeof(double));
            memcpy(prev_u, rtY.u, 5 * sizeof(double));
            memcpy(prev_Buminusd, rtY.Buminusd, 3 * sizeof(double));
            prev_J = rtY.J;
            prev_ridge_on = rtY.ridge_on;
        }
        
        // Generate random inputs for B matrix (3x5, stored column-major)
        for (int j = 0; j < 15; j++) {
            rtU.B_j[j] = nondet_double();
            // Bound inputs to reasonable range
            __ESBMC_assume(rtU.B_j[j] >= -10.0 && rtU.B_j[j] <= 10.0);
        }
        
        // Ensure B has some minimum norm to avoid numerical issues
        double B_norm_sq = 0.0;
        for (int j = 0; j < 15; j++) {
            B_norm_sq += rtU.B_j[j] * rtU.B_j[j];
        }
        __ESBMC_assume(B_norm_sq > 0.01);
        
        // Run the system step
        EB_12B_step();
        
        // PROPERTY 1: When ridge_on is true (det <= 1e-12), check matrix accuracy to 1e-6
        #ifdef VERIFY_PROPERTY_1
        if (rtY.ridge_on) {
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    double check_val = rtY.check[3*col + row];
                    if (row == col) {
                        // Diagonal elements should be close to 1.0
                        __ESBMC_assert(fabs(check_val - 1.0) <= 1.0e-6,
                            "Property 1: Check matrix diagonal accuracy when ridge_on=true");
                    } else {
                        // Off-diagonal elements should be close to 0.0
                        __ESBMC_assert(fabs(check_val - 0.0) <= 1.0e-6,
                            "Property 1: Check matrix off-diagonal accuracy when ridge_on=true");
                    }
                }
            }
        }
        #endif
        
        // PROPERTY 2: When ridge_on is false (det > 1e-12), check matrix accuracy to 1e-12
        #ifdef VERIFY_PROPERTY_2
        if (!rtY.ridge_on) {
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    double check_val = rtY.check[3*col + row];
                    if (row == col) {
                        // Diagonal elements should be close to 1.0
                        __ESBMC_assert(fabs(check_val - 1.0) <= 1.0e-12,
                            "Property 2: Check matrix diagonal accuracy when ridge_on=false");
                    } else {
                        // Off-diagonal elements should be close to 0.0
                        __ESBMC_assert(fabs(check_val - 0.0) <= 1.0e-12,
                            "Property 2: Check matrix off-diagonal accuracy when ridge_on=false");
                    }
                }
            }
        }
        #endif
        
        // PROPERTY 3: The output u vector should be a 5x1 vector
        #ifdef VERIFY_PROPERTY_3
        // Verify u is properly computed as P * d where d = [3; -1; 2]
        for (int k = 0; k < 5; k++) {
            double expected_u = 3.0 * rtY.P_k[k + 5*0] 
                              - 1.0 * rtY.P_k[k + 5*1] 
                              + 2.0 * rtY.P_k[k + 5*2];
            __ESBMC_assert(fabs(rtY.u[k] - expected_u) <= 1.0e-10,
                "Property 3: u vector computed correctly as 5x1");
        }
        #endif
        
        // PROPERTY 4: The 2-norm of Buminusd should be less than 0.01
        #ifdef VERIFY_PROPERTY_4
        double norm_Buminusd = compute_norm2_3x1(rtY.Buminusd);
        __ESBMC_assert(norm_Buminusd < 0.01,
            "Property 4: ||B*u - d||_2 < 0.01");
        #endif
        
        // PROPERTY 5: The output cost J shall be the minimum possible value
        #ifdef VERIFY_PROPERTY_5
        // Verify J is computed as u' * Wp * u
        double J_computed = 0.0;
        for (int m = 0; m < 5; m++) {
            for (int n = 0; n < 5; n++) {
                J_computed += rtY.u[m] * rtConstP.Const_Value[5*n + m] * rtY.u[n];
            }
        }
        __ESBMC_assert(fabs(rtY.J - J_computed) <= 1.0e-10,
            "Property 5: J computed correctly as u'*Wp*u");
        
        // Additional check: When ridge_on is false, verify optimality conditions
        if (!rtY.ridge_on) {
            // For exact solution (no ridge), B*u should equal d within numerical tolerance
            double Bu_minus_d_norm = compute_norm2_3x1(rtY.Buminusd);
            __ESBMC_assert(Bu_minus_d_norm <= 1.0e-8,
                "Property 5: Optimality - B*u = d when ridge_on=false");
        }
        #endif
        
        i++;
    }
    
    return 0;
}
- Uses preprocessing macros (`#ifdef VERIFY_PROPERTY_N`) to control which properties to verify
- Runs the system in a while loop with reasonable iterations
- Saves previous state values (without using static variables)
- Verifies properties after each `EB_12B_step()` execution
- Uses `__ESBMC_assert()` for verification conditions

To verify each property with ESBMC, compile with the appropriate define:
esbmc ert_main.c -DVERIFY_PROPERTY_1
esbmc ert_main.c -DVERIFY_PROPERTY_2
esbmc ert_main.c -DVERIFY_PROPERTY_3
esbmc ert_main.c -DVERIFY_PROPERTY_4
esbmc ert_main.c -DVERIFY_PROPERTY_5