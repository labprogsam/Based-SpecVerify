Looking at the requirements and code, I'll create an assertion-based verification file for ESBMC. Here's the `ert_main.c`:

```c
#include "regs_12B.h"
#include "regs_12B.c"
#include <math.h>

// ESBMC assertion macro
extern void __ESBMC_assert(int cond, const char *msg);

// Non-deterministic inputs
extern _Bool nondet_bool(void);
extern double nondet_double(void);

// Global variables for tracking
double prev_lcvdt_cmd_fcs_dps2;
double prev_mcvdt_cmd_fcs_dps2;
double prev_ncvdt_cmd_fcs_dps2;
double prev_xcvdt_cmd_fcs_fps2;
double prev_hcvdt_cmd_fcs_fps2;

// Persistence counters for amplitude violations
int lcvdt_persist_count;
int mcvdt_persist_count;
int ncvdt_persist_count;
int xcvdt_persist_count;
int hcvdt_persist_count;

// Helper function for absolute value
double abs_double(double x) {
    return (x < 0) ? -x : x;
}

// Initialize inputs with reasonable bounds
void initialize_inputs(void) {
    // Set inputs to reasonable values using non-deterministic choices
    rtU.beta_adc_deg = nondet_double();
    __ESBMC_assume(rtU.beta_adc_deg >= -180.0 && rtU.beta_adc_deg <= 180.0);
    
    rtU.vtas_adc_kts = nondet_double();
    __ESBMC_assume(rtU.vtas_adc_kts >= 0.0 && rtU.vtas_adc_kts <= 300.0);
    
    rtU.lcv_cmd_fcs_dps = nondet_double();
    __ESBMC_assume(rtU.lcv_cmd_fcs_dps >= -100.0 && rtU.lcv_cmd_fcs_dps <= 100.0);
    
    rtU.hdg_des_deg = nondet_double();
    __ESBMC_assume(rtU.hdg_des_deg >= -180.0 && rtU.hdg_des_deg <= 180.0);
    
    rtU.mcv_cmd_fcs_dps = nondet_double();
    __ESBMC_assume(rtU.mcv_cmd_fcs_dps >= -100.0 && rtU.mcv_cmd_fcs_dps <= 100.0);
    
    rtU.alt_des_ft = nondet_double();
    __ESBMC_assume(rtU.alt_des_ft >= -1000.0 && rtU.alt_des_ft <= 50000.0);
    
    rtU.ncv_cmd_fcs_dps = nondet_double();
    __ESBMC_assume(rtU.ncv_cmd_fcs_dps >= -100.0 && rtU.ncv_cmd_fcs_dps <= 100.0);
    
    rtU.xcv_cmd_fcs_fps = nondet_double();
    __ESBMC_assume(rtU.xcv_cmd_fcs_fps >= -500.0 && rtU.xcv_cmd_fcs_fps <= 500.0);
    
    rtU.airspeed_des_fps = nondet_double();
    __ESBMC_assume(rtU.airspeed_des_fps >= 0.0 && rtU.airspeed_des_fps <= 500.0);
    
    rtU.hcv_cmd_fcs_fps = nondet_double();
    __ESBMC_assume(rtU.hcv_cmd_fcs_fps >= -100.0 && rtU.hcv_cmd_fcs_fps <= 100.0);
    
    rtU.lcv_fps_dps = nondet_double();
    __ESBMC_assume(rtU.lcv_fps_dps >= -100.0 && rtU.lcv_fps_dps <= 100.0);
    
    rtU.mcv_fcs_dps = nondet_double();
    __ESBMC_assume(rtU.mcv_fcs_dps >= -100.0 && rtU.mcv_fcs_dps <= 100.0);
    
    rtU.ncv_fcs_dps = nondet_double();
    __ESBMC_assume(rtU.ncv_fcs_dps >= -100.0 && rtU.ncv_fcs_dps <= 100.0);
    
    rtU.dcv_fcs_fps = nondet_double();
    __ESBMC_assume(rtU.dcv_fcs_fps >= -500.0 && rtU.dcv_fcs_fps <= 500.0);
    
    rtU.zcv_fcs_fps = nondet_double();
    __ESBMC_assume(rtU.zcv_fcs_fps >= -100.0 && rtU.zcv_fcs_fps <= 100.0);
    
    rtU.beta_dot = nondet_double();
    __ESBMC_assume(rtU.beta_dot >= -50.0 && rtU.beta_dot <= 50.0);
}

int main(void) {
    // Initialize the model
    regs_12B_initialize();
    
    // Initialize tracking variables
    prev_lcvdt_cmd_fcs_dps2 = 0.0;
    prev_mcvdt_cmd_fcs_dps2 = 0.0;
    prev_ncvdt_cmd_fcs_dps2 = 0.0;
    prev_xcvdt_cmd_fcs_fps2 = 0.0;
    prev_hcvdt_cmd_fcs_fps2 = 0.0;
    
    lcvdt_persist_count = 0;
    mcvdt_persist_count = 0;
    ncvdt_persist_count = 0;
    xcvdt_persist_count = 0;
    hcvdt_persist_count = 0;
    
    // Main loop - bounded for verification
    int cycle = 0;
    int MAX_CYCLES = 150; // Allow for persistence checking over 100 cycles
    
    while (cycle < MAX_CYCLES) {
        // Set inputs for this cycle
        initialize_inputs();
        
        // Store previous outputs for rate checking (only after first cycle)
        if (cycle > 0) {
            prev_lcvdt_cmd_fcs_dps2 = rtY.lcvdt_cmd_fcs_dps2;
            prev_mcvdt_cmd_fcs_dps2 = rtY.mcvdt_cmd_fcs_dps2;
            prev_ncvdt_cmd_fcs_dps2 = rtY.ncvdt_cmd_fcs_dps2;
            prev_xcvdt_cmd_fcs_fps2 = rtY.xcvdt_cmd_fcs_fps2;
            prev_hcvdt_cmd_fcs_fps2 = rtY.hcvdt_cmd_fcs_fps2;
        }
        
        // Execute model step
        regs_12B_step();
        
        // Update persistence counters for amplitude violations
        if (abs_double(rtY.lcvdt_cmd_fcs_dps2) > 50.0) {
            lcvdt_persist_count++;
        } else {
            lcvdt_persist_count = 0;
        }
        
        if (abs_double(rtY.mcvdt_cmd_fcs_dps2) > 50.0) {
            mcvdt_persist_count++;
        } else {
            mcvdt_persist_count = 0;
        }
        
        if (abs_double(rtY.ncvdt_cmd_fcs_dps2) > 50.0) {
            ncvdt_persist_count++;
        } else {
            ncvdt_persist_count = 0;
        }
        
        if (abs_double(rtY.xcvdt_cmd_fcs_fps2) > 32.0) {
            xcvdt_persist_count++;
        } else {
            xcvdt_persist_count = 0;
        }
        
        if (abs_double(rtY.hcvdt_cmd_fcs_fps2) > 32.0) {
            hcvdt_persist_count++;
        } else {
            hcvdt_persist_count = 0;
        }
        
        // PROPERTY 1: Roll amplitude limit - shall not exceed 50 deg/sec2 for more than 1 second
        #ifdef VERIFY_PROPERTY_1
        __ESBMC_assert(lcvdt_persist_count <= 100, 
            "Property 1: Roll amplitude exceeded 50 deg/sec2 for more than 100 cycles");
        #endif
        
        // PROPERTY 2: Pitch amplitude limit - shall not exceed 50 deg/sec2 for more than 1 second
        #ifdef VERIFY_PROPERTY_2
        __ESBMC_assert(mcvdt_persist_count <= 100,
            "Property 2: Pitch amplitude exceeded 50 deg/sec2 for more than 100 cycles");
        #endif
        
        // PROPERTY 3: Yaw amplitude limit - shall not exceed 50 deg/sec2 for more than 1 second
        #ifdef VERIFY_PROPERTY_3
        __ESBMC_assert(ncvdt_persist_count <= 100,
            "Property 3: Yaw amplitude exceeded 50 deg/sec2 for more than 100 cycles");
        #endif
        
        // PROPERTY 4: Airspeed amplitude limit - shall not exceed 32 ft/sec2 for more than 1 second
        #ifdef VERIFY_PROPERTY_4
        __ESBMC_assert(xcvdt_persist_count <= 100,
            "Property 4: Airspeed amplitude exceeded 32 ft/sec2 for more than 100 cycles");
        #endif
        
        // PROPERTY 5: Height amplitude limit - shall not exceed 32 ft/sec2 for more than 1 second
        #ifdef VERIFY_PROPERTY_5
        __ESBMC_assert(hcvdt_persist_count <= 100,
            "Property 5: Height amplitude exceeded 32 ft/sec2 for more than 100 cycles");
        #endif
        
        // Rate limit checks (only after first cycle)
        if (cycle > 0) {
            double lcvdt_rate = rtY.lcvdt_cmd_fcs_dps2 - prev_lcvdt_cmd_fcs_dps2;
            double mcvdt_rate = rtY.mcvdt_cmd_fcs_dps2 - prev_mcvdt_cmd_fcs_dps2;
            double ncvdt_rate = rtY.ncvdt_cmd_fcs_dps2 - prev_ncvdt_cmd_fcs_dps2;
            double xcvdt_rate = rtY.xcvdt_cmd_fcs_fps2 - prev_xcvdt_cmd_fcs_fps2;
            double hcvdt_rate = rtY.hcvdt_cmd_fcs_fps2 - prev_hcvdt_cmd_fcs_fps2;
            
            // PROPERTY 6: Roll rate limit - shall not exceed 50 deg/sec2 per second (0.5 per cycle)
            #ifdef VERIFY_PROPERTY_6
            __ESBMC_assert(abs_double(lcvdt_rate) <= 0.5,
                "Property 6: Roll rate exceeded 0.5 deg/sec2 per cycle");
            #endif
            
            // PROPERTY 7: Pitch rate limit - shall not exceed 50 deg/sec2 per second (0.5 per cycle)
            #ifdef VERIFY_PROPERTY_7
            __ESBMC_assert(abs_double(mcvdt_rate) <= 0.5,
                "Property 7: Pitch rate exceeded 0.5 deg/sec2 per cycle");
            #endif
            
            // PROPERTY 8: Yaw rate limit - shall not exceed 50 deg/sec2 per second (0.5 per cycle)
            #ifdef VERIFY_PROPERTY_8
            __ESBMC_assert(abs_double(ncvdt_rate) <= 0.5,
                "Property 8: Yaw rate exceeded 0.5 deg/sec2 per cycle");
            #endif
            
            // PROPERTY 9: Airspeed rate limit - shall not exceed 32 ft/sec2 per second (0.32 per cycle)
            #ifdef VERIFY_PROPERTY_9
            __ESBMC_assert(abs_double(xcvdt_rate) <= 0.32,
                "Property 9: Airspeed rate exceeded 0.32 ft/sec2 per cycle");
            #endif
            
            // PROPERTY 10: Height rate limit - shall not exceed 32 ft/sec2 per second (0.32 per cycle)
            #ifdef VERIFY_PROPERTY_10
            __ESBMC_assert(abs_double(hcvdt_rate) <= 0.32,
                "Property 10: Height rate exceeded 0.32 ft/sec2 per cycle");
            #endif
        }
        
        cycle++;
    }
    
    return 0;
}
```

This verification code:

1. **Sets up global tracking variables** for previous cycle outputs and persistence counters
2. **Runs the system in a bounded loop** (150 cycles to allow checking 100-cycle persistence)
3. **Uses preprocessing macros** (`VERIFY_PROPERTY_N`) to control which properties to verify
4. **Implements all 10 requirements**:
   - Properties 1-5: Amplitude limits with persistence checking (100 cycles = 1 second at 100Hz)
   - Properties 6-10: Rate limits per cycle (scaled from per-second requirements)
5. **Uses `__ESBMC_assert`** for verification assertions
6. **Uses `nondet_double()` and `nondet_bool()`** for non-deterministic inputs
7. **Tracks previous values** using variables (not static) to compute rate changes
8. **Initializes inputs** with reasonable bounds using `__ESBMC_assume`

To verify a specific property with ESBMC, compile with the appropriate define:
```bash
esbmc ert_main.c -DVERIFY_PROPERTY_1  # For property 1
esbmc ert_main.c -DVERIFY_PROPERTY_2  # For property 2
# ... and so on
```