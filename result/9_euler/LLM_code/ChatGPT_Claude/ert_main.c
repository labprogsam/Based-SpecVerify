/* ert_main.c - Assertion-based verification for euler321_I2B_12B model */

#include "euler321_I2B_12B.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ESBMC nondet functions */
_Bool nondet_bool(void);
double nondet_double(void);

/* Helper function to compute matrix determinant for 3x3 matrix */
double compute_determinant_3x3(const double* M) {
    return M[0] * (M[4] * M[8] - M[5] * M[7]) -
           M[3] * (M[1] * M[8] - M[2] * M[7]) +
           M[6] * (M[1] * M[5] - M[2] * M[4]);
}

/* Helper function to compute dot product */
double dot_product(const double* v1, const double* v2, int size) {
    double result = 0.0;
    for (int i = 0; i < size; i++) {
        result += v1[i] * v2[i];
    }
    return result;
}

/* Helper function to check matrix equality within tolerance */
_Bool matrix_equal(const double* M1, const double* M2, int size, double tol) {
    for (int i = 0; i < size; i++) {
        if (fabs(M1[i] - M2[i]) > tol) {
            return 0;
        }
    }
    return 1;
}

/* Main function with verification properties */
int main(void) {
    const double TOLERANCE = 1e-10;
    const double PI = 3.14159265358979323846;
    const int MAX_ITERATIONS = 5;  /* Reasonable loop bound for verification */
    
    /* Initialize the model */
    euler321_I2B_12B_initialize();
    
    /* Variables to store previous values for comparison */
    double prev_DCM321[9];
    double prev_theta;
    _Bool has_prev = 0;
    
    /* Main verification loop */
    int loop_count = 0;
    while (loop_count < MAX_ITERATIONS) {
        /* Generate non-deterministic inputs */
        rtU.phi = nondet_double();
        rtU.theta = nondet_double();
        rtU.psi = nondet_double();
        
        /* Bound the angles to reasonable values */
        __ESBMC_assume(rtU.phi >= -PI && rtU.phi <= PI);
        __ESBMC_assume(rtU.theta >= -PI && rtU.theta <= PI);
        __ESBMC_assume(rtU.psi >= -PI && rtU.psi <= PI);
        
        /* Generate non-deterministic vector input */
        for (int i = 0; i < 3; i++) {
            rtU.Vi[i] = nondet_double();
            __ESBMC_assume(rtU.Vi[i] >= -100.0 && rtU.Vi[i] <= 100.0);
        }
        
        /* Store previous values if needed */
        if (has_prev) {
            memcpy(prev_DCM321, rtY.DCM321, 9 * sizeof(double));
            prev_theta = rtU.theta;
        }
        
        /* Execute one step of the model */
        euler321_I2B_12B_step();
        
        #ifdef VERIFY_PROPERTY_1
        /* Requirement 1: DCM321 = R3(phi) * R2(theta) * R1(psi) */
        {
            double cos_phi = cos(rtU.phi);
            double sin_phi = sin(rtU.phi);
            double cos_theta = cos(rtU.theta);
            double sin_theta = sin(rtU.theta);
            double cos_psi = cos(rtU.psi);
            double sin_psi = sin(rtU.psi);
            
            /* Expected DCM321 computation */
            double expected_DCM[9];
            expected_DCM[0] = cos_theta * cos_psi;
            expected_DCM[1] = sin_phi * sin_theta * cos_psi - cos_phi * sin_psi;
            expected_DCM[2] = cos_phi * sin_theta * cos_psi + sin_phi * sin_psi;
            expected_DCM[3] = cos_theta * sin_psi;
            expected_DCM[4] = sin_phi * sin_theta * sin_psi + cos_phi * cos_psi;
            expected_DCM[5] = cos_phi * sin_theta * sin_psi - sin_phi * cos_psi;
            expected_DCM[6] = -sin_theta;
            expected_DCM[7] = sin_phi * cos_theta;
            expected_DCM[8] = cos_phi * cos_theta;
            
            for (int i = 0; i < 9; i++) {
                __ESBMC_assert(fabs(rtY.DCM321[i] - expected_DCM[i]) < TOLERANCE,
                              "Property 1: DCM321 correctly computed from Euler angles");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_2
        /* Requirement 2: Vb = DCM321 * Vi */
        {
            double expected_Vb[3];
            for (int i = 0; i < 3; i++) {
                expected_Vb[i] = rtY.DCM321[i] * rtU.Vi[0] + 
                                rtY.DCM321[i + 3] * rtU.Vi[1] + 
                                rtY.DCM321[i + 6] * rtU.Vi[2];
            }
            
            for (int i = 0; i < 3; i++) {
                __ESBMC_assert(fabs(rtY.Vb[i] - expected_Vb[i]) < TOLERANCE,
                              "Property 2: Vb correctly computed as DCM321 * Vi");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3
        /* Requirement 3: Magnitude preservation ||Vb||^2 = ||Vi||^2 */
        {
            double mag_Vi = 0.0;
            double mag_Vb = 0.0;
            
            for (int i = 0; i < 3; i++) {
                mag_Vi += rtU.Vi[i] * rtU.Vi[i];
                mag_Vb += rtY.Vb[i] * rtY.Vb[i];
            }
            
            __ESBMC_assert(fabs(mag_Vb - mag_Vi) < TOLERANCE,
                          "Property 3: Magnitude preservation in transformation");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_4
        /* Requirement 4: DCM321 is invertible (det(DCM321) != 0) */
        {
            double det = compute_determinant_3x3(rtY.DCM321);
            __ESBMC_assert(fabs(det - 1.0) < TOLERANCE,
                          "Property 4: DCM321 has determinant 1 (invertible)");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_5
        /* Requirement 5: Distinct theta produces distinct DCM321 */
        {
            if (has_prev && fabs(rtU.theta - prev_theta) > TOLERANCE) {
                /* Check that DCM321 has changed */
                _Bool matrices_different = 0;
                for (int i = 0; i < 9; i++) {
                    if (fabs(rtY.DCM321[i] - prev_DCM321[i]) > TOLERANCE) {
                        matrices_different = 1;
                        break;
                    }
                }
                __ESBMC_assert(matrices_different,
                              "Property 5: Different theta values produce different DCM321");
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_6
        /* Requirement 6: DCM321 has orthonormal rows and columns */
        {
            /* Check row orthonormality */
            for (int i = 0; i < 3; i++) {
                double row[3];
                for (int j = 0; j < 3; j++) {
                    row[j] = rtY.DCM321[i + 3 * j];  /* Row i */
                }
                
                /* Check norm = 1 */
                double norm = dot_product(row, row, 3);
                __ESBMC_assert(fabs(norm - 1.0) < TOLERANCE,
                              "Property 6a: DCM321 rows have unit norm");
                
                /* Check orthogonality with other rows */
                for (int k = i + 1; k < 3; k++) {
                    double other_row[3];
                    for (int j = 0; j < 3; j++) {
                        other_row[j] = rtY.DCM321[k + 3 * j];
                    }
                    double dot = dot_product(row, other_row, 3);
                    __ESBMC_assert(fabs(dot) < TOLERANCE,
                                  "Property 6b: DCM321 rows are orthogonal");
                }
            }
            
            /* Check column orthonormality */
            for (int j = 0; j < 3; j++) {
                double col[3];
                for (int i = 0; i < 3; i++) {
                    col[i] = rtY.DCM321[i + 3 * j];  /* Column j */
                }
                
                /* Check norm = 1 */
                double norm = dot_product(col, col, 3);
                __ESBMC_assert(fabs(norm - 1.0) < TOLERANCE,
                              "Property 6c: DCM321 columns have unit norm");
                
                /* Check orthogonality with other columns */
                for (int k = j + 1; k < 3; k++) {
                    double other_col[3];
                    for (int i = 0; i < 3; i++) {
                        other_col[i] = rtY.DCM321[i + 3 * k];
                    }
                    double dot = dot_product(col, other_col, 3);
                    __ESBMC_assert(fabs(dot) < TOLERANCE,
                                  "Property 6d: DCM321 columns are orthogonal");
                }
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_7
        /* Requirement 7: DCM321 * DCM321^T = I */
        {
            double product[9];
            
            /* Compute DCM321 * DCM321^T */
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    product[i + 3 * j] = 0.0;
                    for (int k = 0; k < 3; k++) {
                        product[i + 3 * j] += rtY.DCM321[i + 3 * k] * rtY.DCM321[j + 3 * k];
                    }
                }
            }
            
            /* Check if product equals identity matrix */
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    double expected = (i == j) ? 1.0 : 0.0;
                    __ESBMC_assert(fabs(product[i + 3 * j] - expected) < TOLERANCE,
                                  "Property 7: DCM321 * DCM321^T = Identity");
                }
            }
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_8
        /* Requirement 8: det(DCM321) = 1 */
        {
            double det = compute_determinant_3x3(rtY.DCM321);
            __ESBMC_assert(fabs(det - 1.0) < TOLERANCE,
                          "Property 8: Determinant of DCM321 equals 1");
        }
        #endif
        
        /* Mark that we have previous values for next iteration */
        has_prev = 1;
        loop_count++;
    }
    
    return 0;
}
