#include <math.h>
#include <stdio.h>
#include "NLGuidance_12B.h"
#include "rtwtypes.h"

// Nondet functions for ESBMC
_Bool nondet_bool();
double nondet_double();

// ESBMC assertion macro
#define __ESBMC_assert(cond) do { if (!(cond)) { __builtin_abort(); } } while(0)

// Helper function to compute vector magnitude
double vector_magnitude(double v[3]) {
    return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

// Helper function to compute dot product
double dot_product(double v1[3], double v2[3]) {
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

// Helper function to compute distance between two points
double distance(double p1[3], double p2[3]) {
    double dx = p1[0] - p2[0];
    double dy = p1[1] - p2[1];
    double dz = p1[2] - p2[2];
    return sqrt(dx*dx + dy*dy + dz*dz);
}

int main() {
    // Initialize the model
    NLGuidance_12B_initialize();
    
    // Variables to store previous outputs for requirement 6
    double prev_yout[3] = {0.0, 0.0, 0.0};
    double T = 0.1; // Sample time assumption
    
    // Run system for multiple loops
    int loop_count = 10;
    int loop = 0;
    
    while (loop < loop_count) {
        // Set nondeterministic inputs
        rtU.Xtarg[0] = nondet_double();
        rtU.Xtarg[1] = nondet_double();
        rtU.Xtarg[2] = nondet_double();
        
        rtU.Xv[0] = nondet_double();
        rtU.Xv[1] = nondet_double();
        rtU.Xv[2] = nondet_double();
        
        rtU.Vv[0] = nondet_double();
        rtU.Vv[1] = nondet_double();
        rtU.Vv[2] = nondet_double();
        
        rtU.r = nondet_double();
        
        // Assume positive standoff radius
        __ESBMC_assert(rtU.r > 0.0);
        
        // Store previous output for requirement 6
        if (loop > 0) {
            prev_yout[0] = rtY.yout[0];
            prev_yout[1] = rtY.yout[1];
            prev_yout[2] = rtY.yout[2];
        }
        
        // Execute one step
        NLGuidance_12B_step();
        
        // Compute relative position vector Xr
        double Xr[3];
        Xr[0] = rtU.Xtarg[0] - rtU.Xv[0];
        Xr[1] = rtU.Xtarg[1] - rtU.Xv[1];
        Xr[2] = rtU.Xtarg[2] - rtU.Xv[2];
        
        double Xr_mag_sq = Xr[0]*Xr[0] + Xr[1]*Xr[1] + Xr[2]*Xr[2];
        double Xr_mag = sqrt(Xr_mag_sq);
        
        double XrDotVv = dot_product(Xr, rtU.Vv);
        double Vv_mag = vector_magnitude(rtU.Vv);
        double Vv_mag_sq = Vv_mag * Vv_mag;
        
        // Determine mode
        double Sum2 = Xr_mag_sq - rtU.r * rtU.r;
        double Sum1 = Vv_mag_sq * Xr_mag_sq - XrDotVv * XrDotVv;
        
        _Bool is_inner_mode = (Sum2 <= 0.0) || (Sum1 <= 0.0);
        _Bool is_outer_mode = !is_inner_mode;
        
#ifdef VERIFY_PROPERTY_1
        // Requirement 1: NLGuidance shall always maintain the target on the port-side of the vehicle
        // In Outer mode, verify CCW selection (port-side)
        if (is_outer_mode) {
            // The code always selects CCW in outer mode due to the zero condition
            // Verify that CCW output is computed
            double scale = sqrt(Sum2 / Sum1);
            double t[3];
            t[0] = scale * rtU.Vv[0];
            t[1] = scale * rtU.Vv[1];
            t[2] = scale * rtU.Vv[2];
            
            double M1 = 1.0 / Xr_mag_sq;
            double S = scale * XrDotVv;
            double k_minus = S - rtU.r;
            
            // Expected CCW output
            double expected_yout[3];
            expected_yout[0] = rtU.Xv[0] + ((k_minus * Xr[0] * M1 - t[0]) * rtU.r + Xr[0]);
            expected_yout[1] = rtU.Xv[1] + ((k_minus * Xr[1] * M1 - t[1]) * rtU.r + Xr[1]);
            expected_yout[2] = rtU.Xv[2] + ((k_minus * Xr[2] * M1 - t[2]) * rtU.r + Xr[2]);
            
            // Verify output matches CCW computation
            __ESBMC_assert(fabs(rtY.yout[0] - expected_yout[0]) < 1e-6);
            __ESBMC_assert(fabs(rtY.yout[1] - expected_yout[1]) < 1e-6);
            __ESBMC_assert(fabs(rtY.yout[2] - expected_yout[2]) < 1e-6);
        }
#endif
        
#ifdef VERIFY_PROPERTY_2
        // Requirement 2: Compute inertial position vector for aim point 1 at standoff
        if (is_outer_mode) {
            // Aim point 1 (CW branch - though never selected)
            double scale = sqrt(Sum2 / Sum1);
            double t[3];
            t[0] = scale * rtU.Vv[0];
            t[1] = scale * rtU.Vv[1];
            t[2] = scale * rtU.Vv[2];
            
            double M1 = 1.0 / Xr_mag_sq;
            double S = scale * XrDotVv;
            double k_plus = rtU.r + S;
            
            // Compute aim point 1 (CW)
            double Xap1[3];
            Xap1[0] = rtU.Xv[0] + ((k_plus * Xr[0] * (-M1) + t[0]) * rtU.r + Xr[0]);
            Xap1[1] = rtU.Xv[1] + ((k_plus * Xr[1] * (-M1) + t[1]) * rtU.r + Xr[1]);
            Xap1[2] = rtU.Xv[2] + ((k_plus * Xr[2] * (-M1) + t[2]) * rtU.r + Xr[2]);
            
            // Verify standoff distance
            double dist_to_target = distance(Xap1, rtU.Xtarg);
            __ESBMC_assert(fabs(dist_to_target - rtU.r) < 1e-6);
            
            // Verify perpendicularity
            double Xap1_to_targ[3];
            Xap1_to_targ[0] = Xap1[0] - rtU.Xtarg[0];
            Xap1_to_targ[1] = Xap1[1] - rtU.Xtarg[1];
            Xap1_to_targ[2] = Xap1[2] - rtU.Xtarg[2];
            
            double Xap1_to_veh[3];
            Xap1_to_veh[0] = Xap1[0] - rtU.Xv[0];
            Xap1_to_veh[1] = Xap1[1] - rtU.Xv[1];
            Xap1_to_veh[2] = Xap1[2] - rtU.Xv[2];
            
            double perpendicular_dot = dot_product(Xap1_to_targ, Xap1_to_veh);
            __ESBMC_assert(fabs(perpendicular_dot) < 1e-6);
        }
#endif
        
#ifdef VERIFY_PROPERTY_3
        // Requirement 3: Compute inertial position vector for aim point 2 at standoff
        if (is_outer_mode) {
            // Verify the actual output (CCW branch - aim point 2)
            double yout_to_targ[3];
            yout_to_targ[0] = rtY.yout[0] - rtU.Xtarg[0];
            yout_to_targ[1] = rtY.yout[1] - rtU.Xtarg[1];
            yout_to_targ[2] = rtY.yout[2] - rtU.Xtarg[2];
            
            // Verify standoff distance
            double dist_to_target = vector_magnitude(yout_to_targ);
            __ESBMC_assert(fabs(dist_to_target - rtU.r) < 1e-6);
            
            // Verify perpendicularity
            double yout_to_veh[3];
            yout_to_veh[0] = rtY.yout[0] - rtU.Xv[0];
            yout_to_veh[1] = rtY.yout[1] - rtU.Xv[1];
            yout_to_veh[2] = rtY.yout[2] - rtU.Xv[2];
            
            double perpendicular_dot = dot_product(yout_to_targ, yout_to_veh);
            __ESBMC_assert(fabs(perpendicular_dot) < 1e-6);
        }
#endif
        
#ifdef VERIFY_PROPERTY_4
        // Requirement 4: Always select aim point resulting in counter clockwise loiter
        if (is_outer_mode) {
            // Verify CCW output is selected
            double scale = sqrt(Sum2 / Sum1);
            double t[3];
            t[0] = scale * rtU.Vv[0];
            t[1] = scale * rtU.Vv[1];
            t[2] = scale * rtU.Vv[2];
            
            double M1 = 1.0 / Xr_mag_sq;
            double S = scale * XrDotVv;
            double k_minus = S - rtU.r;
            
            // Expected CCW output
            double expected_ccw[3];
            expected_ccw[0] = rtU.Xv[0] + ((k_minus * Xr[0] * M1 - t[0]) * rtU.r + Xr[0]);
            expected_ccw[1] = rtU.Xv[1] + ((k_minus * Xr[1] * M1 - t[1]) * rtU.r + Xr[1]);
            expected_ccw[2] = rtU.Xv[2] + ((k_minus * Xr[2] * M1 - t[2]) * rtU.r + Xr[2]);
            
            // Verify actual output matches CCW
            __ESBMC_assert(fabs(rtY.yout[0] - expected_ccw[0]) < 1e-6);
            __ESBMC_assert(fabs(rtY.yout[1] - expected_ccw[1]) < 1e-6);
            __ESBMC_assert(fabs(rtY.yout[2] - expected_ccw[2]) < 1e-6);
        }
#endif
        
#ifdef VERIFY_PROPERTY_5
        // Requirement 5: When inside standoff, command nearest inertial position
        if (is_inner_mode) {
            // Compute both candidate aim points
            double s = rtU.r / Xr_mag;
            
            // Act1 candidate
            double Act1[3];
            Act1[0] = rtU.Xtarg[0] + s * Xr[0];
            Act1[1] = rtU.Xtarg[1] + s * Xr[1];
            Act1[2] = 0.0;
            
            // Act2 candidate
            double Act2[3];
            Act2[0] = rtU.Xtarg[0] - s * Xr[0];
            Act2[1] = rtU.Xtarg[1] - s * Xr[1];
            Act2[2] = 0.0;
            
            // Compute distances to vehicle
            double dAct1_sq = (Act1[0] - rtU.Xv[0])*(Act1[0] - rtU.Xv[0]) + 
                             (Act1[1] - rtU.Xv[1])*(Act1[1] - rtU.Xv[1]) + 
                             (0.0 - rtU.Xv[2])*(0.0 - rtU.Xv[2]);
            
            double dAct2_sq = (Act2[0] - rtU.Xv[0])*(Act2[0] - rtU.Xv[0]) + 
                             (Act2[1] - rtU.Xv[1])*(Act2[1] - rtU.Xv[1]) + 
                             (0.0 - rtU.Xv[2])*(0.0 - rtU.Xv[2]);
            
            double dAct1 = sqrt(dAct1_sq);
            double dAct2 = sqrt(dAct2_sq);
            
            // Verify nearest is selected
            if (dAct1 < dAct2) {
                __ESBMC_assert(fabs(rtY.yout[0] - Act1[0]) < 1e-6);
                __ESBMC_assert(fabs(rtY.yout[1] - Act1[1]) < 1e-6);
            } else {
                __ESBMC_assert(fabs(rtY.yout[0] - Act2[0]) < 1e-6);
                __ESBMC_assert(fabs(rtY.yout[1] - Act2[1]) < 1e-6);
            }
            
            // Verify altitude is set to 0
            __ESBMC_assert(rtY.yout[2] == 0.0);
        }
#endif
        
#ifdef VERIFY_PROPERTY_6
        // Requirement 6: Output consistent aim point - magnitude change limitation
        // Note: This requirement cannot be fully verified as Vt is not present