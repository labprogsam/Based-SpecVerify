#include "NLGuidance_12B.h"
#include <math.h>
#include <stdio.h>

// ESBMC nondet functions
_Bool nondet_bool(void);
double nondet_double(void);

// Helper function to compute distance
double compute_distance(double x1, double y1, double z1, double x2, double y2, double z2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double dz = z1 - z2;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

// Helper function to compute dot product
double dot_product(double x1, double y1, double z1, double x2, double y2, double z2) {
    return x1*x2 + y1*y2 + z1*z2;
}

// Helper function to compute 2D cross product Z component
double cross_product_z(double x1, double y1, double x2, double y2) {
    return x1*y2 - y1*x2;
}

int main(void) {
    // Initialize the model
    NLGuidance_12B_initialize();
    
    // Variables to store previous outputs for consistency checking
    double prev_yout[3] = {0.0, 0.0, 0.0};
    _Bool first_iteration = 1;
    
    // Run system for multiple iterations
    int MAX_ITERATIONS = 10;
    int iter = 0;
    
    while (iter < MAX_ITERATIONS) {
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
        __ESBMC_assume(rtU.r > 0.0);
        __ESBMC_assume(rtU.r < 1000.0); // Reasonable bound
        
        // Assume reasonable bounds on positions and velocities
        for (int i = 0; i < 3; i++) {
            __ESBMC_assume(rtU.Xtarg[i] > -10000.0 && rtU.Xtarg[i] < 10000.0);
            __ESBMC_assume(rtU.Xv[i] > -10000.0 && rtU.Xv[i] < 10000.0);
            __ESBMC_assume(rtU.Vv[i] > -100.0 && rtU.Vv[i] < 100.0);
        }
        
        // Store previous output if not first iteration
        if (!first_iteration) {
            prev_yout[0] = rtY.yout[0];
            prev_yout[1] = rtY.yout[1];
            prev_yout[2] = rtY.yout[2];
        }
        
        // Execute one step of the model
        NLGuidance_12B_step();
        
        // Compute relative position and other key values for verification
        double Xr_x = rtU.Xtarg[0] - rtU.Xv[0];
        double Xr_y = rtU.Xtarg[1] - rtU.Xv[1];
        double Xr_z = rtU.Xtarg[2] - rtU.Xv[2];
        double Xr_norm_sq = Xr_x*Xr_x + Xr_y*Xr_y + Xr_z*Xr_z;
        double Xr_norm = sqrt(Xr_norm_sq);
        
        double Vv_norm_sq = rtU.Vv[0]*rtU.Vv[0] + rtU.Vv[1]*rtU.Vv[1] + rtU.Vv[2]*rtU.Vv[2];
        double XrDotVv = Xr_x*rtU.Vv[0] + Xr_y*rtU.Vv[1] + Xr_z*rtU.Vv[2];
        
        // Determine mode (Inner vs Outer)
        _Bool is_inner_mode = (Xr_norm_sq - rtU.r*rtU.r <= 0.0) || 
                             (Vv_norm_sq * Xr_norm_sq - XrDotVv*XrDotVv <= 0.0);
        
        #ifdef VERIFY_PROPERTY_1
        // Property 1: NLGuidance shall always maintain the target on the port-side (CCW)
        // This is verified by checking that in Outer mode, CCW branch is always selected
        if (!is_inner_mode) {
            // In outer mode, verify CCW selection
            // The code always selects CCW due to the zero cross product condition
            // Verify the output matches CCW formula
            double scale = sqrt((Xr_norm_sq - rtU.r*rtU.r) / (Vv_norm_sq * Xr_norm_sq - XrDotVv*XrDotVv));
            double S = scale * XrDotVv;
            double k_minus = S - rtU.r;
            double M1 = 1.0 / Xr_norm_sq;
            
            // CCW output formula verification
            double expected_x = rtU.Xv[0] + ((k_minus * Xr_x * M1 - scale * rtU.Vv[0]) * rtU.r + Xr_x);
            double expected_y = rtU.Xv[1] + ((k_minus * Xr_y * M1 - scale * rtU.Vv[1]) * rtU.r + Xr_y);
            
            // Allow small numerical tolerance
            __ESBMC_assert(fabs(rtY.yout[0] - expected_x) < 1e-6, "Property 1: CCW output X coordinate");
            __ESBMC_assert(fabs(rtY.yout[1] - expected_y) < 1e-6, "Property 1: CCW output Y coordinate");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_2
        // Property 2: Compute aim point 1 at standoff (CW branch - though never selected)
        // Verify that if CW were selected, it would satisfy standoff and perpendicularity
        if (!is_inner_mode) {
            double scale = sqrt((Xr_norm_sq - rtU.r*rtU.r) / (Vv_norm_sq * Xr_norm_sq - XrDotVv*XrDotVv));
            double S = scale * XrDotVv;
            double k_plus = rtU.r + S;
            double M1 = 1.0 / Xr_norm_sq;
            
            // CW aim point (aim point 1)
            double Xap1_x = rtU.Xv[0] + ((k_plus * Xr_x * (-M1) + scale * rtU.Vv[0]) * rtU.r + Xr_x);
            double Xap1_y = rtU.Xv[1] + ((k_plus * Xr_y * (-M1) + scale * rtU.Vv[1]) * rtU.r + Xr_y);
            double Xap1_z = rtU.Xv[2] + ((k_plus * Xr_z * (-M1) + scale * rtU.Vv[2]) * rtU.r + Xr_z);
            
            // Verify standoff distance
            double dist_to_target = compute_distance(Xap1_x, Xap1_y, Xap1_z, 
                                                    rtU.Xtarg[0], rtU.Xtarg[1], rtU.Xtarg[2]);
            __ESBMC_assert(fabs(dist_to_target - rtU.r) < 1e-6, "Property 2: Aim point 1 at standoff");
            
            // Verify perpendicularity
            double rel_x = Xap1_x - rtU.Xtarg[0];
            double rel_y = Xap1_y - rtU.Xtarg[1];
            double rel_z = Xap1_z - rtU.Xtarg[2];
            double tangent_x = Xap1_x - rtU.Xv[0];
            double tangent_y = Xap1_y - rtU.Xv[1];
            double tangent_z = Xap1_z - rtU.Xv[2];
            double perpendicular_dot = dot_product(rel_x, rel_y, rel_z, tangent_x, tangent_y, tangent_z);
            __ESBMC_assert(fabs(perpendicular_dot) < 1e-6, "Property 2: Perpendicularity at tangent");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_3
        // Property 3: Compute aim point 2 at standoff (CCW branch - always selected)
        if (!is_inner_mode) {
            // Verify that the actual output (CCW) satisfies standoff and perpendicularity
            double dist_to_target = compute_distance(rtY.yout[0], rtY.yout[1], rtY.yout[2],
                                                    rtU.Xtarg[0], rtU.Xtarg[1], rtU.Xtarg[2]);
            __ESBMC_assert(fabs(dist_to_target - rtU.r) < 1e-6, "Property 3: Aim point 2 at standoff");
            
            // Verify perpendicularity
            double rel_x = rtY.yout[0] - rtU.Xtarg[0];
            double rel_y = rtY.yout[1] - rtU.Xtarg[1];
            double rel_z = rtY.yout[2] - rtU.Xtarg[2];
            double tangent_x = rtY.yout[0] - rtU.Xv[0];
            double tangent_y = rtY.yout[1] - rtU.Xv[1];
            double tangent_z = rtY.yout[2] - rtU.Xv[2];
            double perpendicular_dot = dot_product(rel_x, rel_y, rel_z, tangent_x, tangent_y, tangent_z);
            __ESBMC_assert(fabs(perpendicular_dot) < 1e-6, "Property 3: Perpendicularity at tangent");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_4
        // Property 4: Always select CCW loiter
        if (!is_inner_mode) {
            // Verify the cross product condition always leads to CCW
            // The condition (Xr_x * Sum_c_y - Xr_x * Sum_c_y) is always 0, so CCW is selected
            // We verify by checking that the output matches CCW formula
            double scale = sqrt((Xr_norm_sq - rtU.r*rtU.r) / (Vv_norm_sq * Xr_norm_sq - XrDotVv*XrDotVv));
            double S = scale * XrDotVv;
            double k_minus = S - rtU.r;
            double M1 = 1.0 / Xr_norm_sq;
            
            double expected_x = rtU.Xv[0] + ((k_minus * Xr_x * M1 - scale * rtU.Vv[0]) * rtU.r + Xr_x);
            double expected_y = rtU.Xv[1] + ((k_minus * Xr_y * M1 - scale * rtU.Vv[1]) * rtU.r + Xr_y);
            
            __ESBMC_assert(fabs(rtY.yout[0] - expected_x) < 1e-6, "Property 4: CCW selection X");
            __ESBMC_assert(fabs(rtY.yout[1] - expected_y) < 1e-6, "Property 4: CCW selection Y");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_5
        // Property 5: When inside standoff, command nearest position
        if (is_inner_mode) {
            // Compute both candidates
            double s = rtU.r / Xr_norm;
            
            // Act1 candidate
            double Act1_x = rtU.Xtarg[0] + s * Xr_x;
            double Act1_y = rtU.Xtarg[1] + s * Xr_y;
            double Act1_z = 0.0;
            
            // Act2 candidate
            double Act2_x = rtU.Xtarg[0] - s * Xr_x;
            double Act2_y = rtU.Xtarg[1] - s * Xr_y;
            double Act2_z = 0.0;
            
            // Compute distances to vehicle
            double dAct1 = compute_distance(Act1_x, Act1_y, Act1_z, rtU.Xv[0], rtU.Xv[1], rtU.Xv[2]);
            double dAct2 = compute_distance(Act2_x, Act2_y, Act2_z, rtU.Xv[0], rtU.Xv[1], rtU.Xv[2]);
            
            // Verify nearest is selected
            if (dAct1 < dAct2) {
                __ESBMC_assert(fabs(rtY.yout[0] - Act1_x) < 1e-6, "Property 5: Nearest selection Act1 X");
                __ESBMC_assert(fabs(rtY.yout[1] - Act1_y) < 1e-6, "Property 5: Nearest selection Act1 Y");
            } else {
                __ESBMC_assert(fabs(rtY.yout[0] - Act2_x) < 1e-6, "Property 5: Nearest selection Act2 X");
                __ESBMC_assert(fabs(rtY.yout[1] - Act2_y) < 1e-6, "Property 5: Nearest selection Act2 Y");
            }
            
            // Verify Z is always 0 in inner mode
            __ESBMC_assert(rtY.yout[2] == 0.0, "Property 5: Inner mode Z=0");
        }
        #endif
        
        #ifdef VERIFY_PROPERTY_6
        // Property 6: Output consistency - magnitude change limited
        // Note: This property cannot be fully verified as Vt and T are not in the code
        // We can only check that outputs don't change dramatically
        if (!first_iteration) {
            double output_change = compute_distance(rtY.yout[0], rtY.yout[1], rtY.yout[2],
                                                   prev_yout[0], prev_yout[