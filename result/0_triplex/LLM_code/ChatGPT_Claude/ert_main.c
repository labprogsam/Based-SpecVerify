#include "triplex_12B.h"
#include "triplex_12B.c"
#include <math.h>
#include <stdlib.h>

// ESBMC assertion macro
void __ESBMC_assert(_Bool cond, const char* msg);

// Nondeterministic inputs
double nondet_double(void);
int32_T nondet_int32(void);
_Bool nondet_bool(void);

// Helper function to check if value is approximately mid-value
_Bool is_mid_value(double val, double a, double b, double c) {
    double sorted[3] = {a, b, c};
    // Simple bubble sort
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2 - i; j++) {
            if (sorted[j] > sorted[j + 1]) {
                double temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    // Check if val is the middle value (with small tolerance for floating point)
    return fabs(val - sorted[1]) < 0.0001;
}

int main(void) {
    // Initialize the system
    triplex_12B_initialize();
    
    // Set initial state
    rtDW.Delay1_DSTATE[0] = 0;  // PC starts at 0
    rtDW.Delay1_DSTATE[1] = 0;  // TC starts at 0
    rtDW.Delay1_DSTATE[2] = 0;  // FC starts at 0 (no-fail)
    
    // Set persistence limit
    rtU.PClimit = 3;  // Small value for verification
    
    // Loop counter
    int loop_count = 0;
    int max_loops = 20;  // Reasonable number for verification
    
    // Variables to track previous values (not static)
    double prev_sel_val = 0.0;
    int32_T prev_PC = 0;
    int32_T prev_TC = 0;
    int32_T prev_FC = 0;
    
    while (loop_count < max_loops) {
        // Store previous values before step
        prev_PC = rtDW.Delay1_DSTATE[0];
        prev_TC = rtDW.Delay1_DSTATE[1];
        prev_FC = rtDW.Delay1_DSTATE[2];
        prev_sel_val = rtY.sel_val;
        
        // Set nondeterministic inputs
        rtU.ia = nondet_double();
        rtU.ib = nondet_double();
        rtU.ic = nondet_double();
        rtU.Tlevel = nondet_double();
        
        // Constrain inputs to reasonable ranges
        __ESBMC_assume(rtU.ia >= -100.0 && rtU.ia <= 100.0);
        __ESBMC_assume(rtU.ib >= -100.0 && rtU.ib <= 100.0);
        __ESBMC_assume(rtU.ic >= -100.0 && rtU.ic <= 100.0);
        __ESBMC_assume(rtU.Tlevel >= 0.1 && rtU.Tlevel <= 10.0);
        
        // Run one step
        triplex_12B_step();
        
        // Calculate miscompare conditions
        _Bool Rel_AB = fabs(rtU.ia - rtU.ib) > rtU.Tlevel;
        _Bool Rel_BC = fabs(rtU.ib - rtU.ic) > rtU.Tlevel;
        _Bool Rel_AC = fabs(rtU.ia - rtU.ic) > rtU.Tlevel;
        _Bool no_mis = (!Rel_AB && !Rel_BC && !Rel_AC);
        
        // Single-branch isolation patterns
        _Bool A_fail = Rel_AB && !Rel_BC && Rel_AC;
        _Bool B_fail = Rel_AB && Rel_BC && !Rel_AC;
        _Bool C_fail = !Rel_AB && Rel_BC && Rel_AC;
        _Bool single_fail = A_fail || B_fail || C_fail;
        
        // All-three miscompare
        _Bool all_three_mis = Rel_AB && Rel_BC && Rel_AC;
        
#ifdef VERIFY_PROPERTY_1
        // RM-001: First failure detection and latching
        if (single_fail) {
            if (prev_PC <= rtU.PClimit) {
                // Pending failure: PC and TC should increment
                __ESBMC_assert(rtY.PC == prev_PC + 1, 
                    "RM-001: PC should increment during pending failure");
                __ESBMC_assert(rtY.TC == prev_TC + 1, 
                    "RM-001: TC should increment during pending failure");
                __ESBMC_assert(rtY.FC == prev_FC, 
                    "RM-001: FC should remain unchanged during pending");
            }
            
            if (prev_PC > rtU.PClimit) {
                // Latching should occur
                __ESBMC_assert(rtY.PC == 0, 
                    "RM-001: PC should reset to 0 on latch");
                __ESBMC_assert(rtY.TC == prev_TC, 
                    "RM-001: TC should not change on latch");
                
                // Check correct fault code
                if (A_fail) {
                    __ESBMC_assert(rtY.FC == 4, 
                        "RM-001: FC should be 4 for A-fail");
                } else if (B_fail) {
                    __ESBMC_assert(rtY.FC == 2, 
                        "RM-001: FC should be 2 for B-fail");
                } else if (C_fail) {
                    __ESBMC_assert(rtY.FC == 1, 
                        "RM-001: FC should be 1 for C-fail");
                }
            }
        }
#endif
        
#ifdef VERIFY_PROPERTY_2
        // RM-002: Mid-value selection when FC == 0
        if (rtY.FC == 0) {
            // Verify mid-value selection
            _Bool is_mid = is_mid_value(rtY.sel_val, rtU.ia, rtU.ib, rtU.ic);
            __ESBMC_assert(is_mid, 
                "RM-002: sel_val should be mid-value when FC == 0");
        }
#endif
        
#ifdef VERIFY_PROPERTY_3
        // RM-003: Good Channel Average when single failure
        if (rtY.FC == 1) {
            // C failed, should average A and B
            double expected = (rtU.ia + rtU.ib) / 2.0;
            __ESBMC_assert(fabs(rtY.sel_val - expected) < 0.0001, 
                "RM-003: sel_val should be average of ia and ib when FC == 1");
        }
        
        if (rtY.FC == 2) {
            // B failed, should average A and C
            double expected = (rtU.ia + rtU.ic) / 2.0;
            __ESBMC_assert(fabs(rtY.sel_val - expected) < 0.0001, 
                "RM-003: sel_val should be average of ia and ic when FC == 2");
        }
        
        if (rtY.FC == 4) {
            // A failed, should average B and C
            double expected = (rtU.ib + rtU.ic) / 2.0;
            __ESBMC_assert(fabs(rtY.sel_val - expected) < 0.0001, 
                "RM-003: sel_val should be average of ib and ic when FC == 4");
        }
#endif
        
#ifdef VERIFY_PROPERTY_4
        // RM-004: Second failure in progress behavior
        if (all_three_mis) {
            // PC and FC should remain unchanged
            __ESBMC_assert(rtY.PC == prev_PC, 
                "RM-004: PC should remain unchanged during all-three miscompare");
            __ESBMC_assert(rtY.FC == prev_FC, 
                "RM-004: FC should remain unchanged during all-three miscompare");
            
            // TC should increment
            __ESBMC_assert(rtY.TC == prev_TC + 1, 
                "RM-004: TC should increment during all-three miscompare");
            
            // If already in single-fail state, selection should continue per FC
            if (prev_FC != 0) {
                // Verify selection continues based on FC (not frozen to numeric value)
                if (rtY.FC == 1) {
                    double expected = (rtU.ia + rtU.ib) / 2.0;
                    __ESBMC_assert(fabs(rtY.sel_val - expected) < 0.0001, 
                        "RM-004: Selection should continue as GCA during second failure");
                } else if (rtY.FC == 2) {
                    double expected = (rtU.ia + rtU.ic) / 2.0;
                    __ESBMC_assert(fabs(rtY.sel_val - expected) < 0.0001, 
                        "RM-004: Selection should continue as GCA during second failure");
                } else if (rtY.FC == 4) {
                    double expected = (rtU.ib + rtU.ic) / 2.0;
                    __ESBMC_assert(fabs(rtY.sel_val - expected) < 0.0001, 
                        "RM-004: Selection should continue as GCA during second failure");
                }
            }
        }
#endif
        
        // Additional state transition checks (always active)
        
        // Check quiet behavior
        if (no_mis) {
            if (prev_PC == 0) {
                __ESBMC_assert(rtY.PC == 0, "PC should stay 0 when quiet and PC was 0");
                __ESBMC_assert(rtY.TC == prev_TC, "TC unchanged when quiet");
                __ESBMC_assert(rtY.FC == prev_FC, "FC unchanged when quiet");
            } else if (prev_PC > 0) {
                __ESBMC_assert(rtY.PC == prev_PC - 1, "PC should decay when quiet and PC > 0");
                __ESBMC_assert(rtY.TC == prev_TC, "TC unchanged during decay");
                __ESBMC_assert(rtY.FC == prev_FC, "FC unchanged during decay");
            }
        }
        
        // Check exactly-one-pair miscompare behavior
        _Bool exactly_one_pair = (Rel_AB && !Rel_BC && !Rel_AC) ||
                                  (!Rel_AB && Rel_BC && !Rel_AC) ||
                                  (!Rel_AB && !Rel_BC && Rel_AC);
        if (exactly_one_pair) {
            __ESBMC_assert(rtY.PC == prev_PC, "PC unchanged for exactly-one-pair miscompare");
            __ESBMC_assert(rtY.TC == prev_TC, "TC unchanged for exactly-one-pair miscompare");
            __ESBMC_assert(rtY.FC == prev_FC, "FC unchanged for exactly-one-pair miscompare");
        }
        
        // Verify FC values are valid
        __ESBMC_assert(rtY.FC == 0 || rtY.FC == 1 || rtY.FC == 2 || rtY.FC == 4,
            "FC must be one of {0, 1, 2, 4}");
        
        // Verify PC and TC are non-negative
        __ESBMC_assert(rtY.PC >= 0, "PC must be non-negative");
        __ESBMC_assert(rtY.TC >= 0, "TC must be non-negative");
        
        loop_count++;
    }
    
    return 0;
}
