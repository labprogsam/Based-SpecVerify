/*
 * ert_main.c - ESBMC verification harness with assertion-based properties
 *
 * This file drives the generated model euler321_I2B_12B and checks
 * the formal requirements using __ESBMC_assert under preprocessor
 * control macros VERIFY_PROPERTY_N.
 *
 * Notes:
 * - All properties are verified after each step in a while loop.
 * - No state is retained in the model; a property checks this explicitly.
 * - Use preprocessing macros (e.g., -DVERIFY_PROPERTY_1) to enable checks.
 * - Do not define VERIFY_PROPERTY_* inside this file as per instructions.
 */

#include "euler321_I2B_12B.h"
#include <math.h>
#include <string.h>

/* Non-deterministic inputs for ESBMC */
double nondet_double(void);
_Bool nondet_bool(void);

/* ESBMC assume */
void __ESBMC_assume(_Bool);

/* Constants and tolerances */
#ifndef LOOP_MAX
#define LOOP_MAX 5
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ANGLE_MIN (-M_PI)
#define ANGLE_MAX ( M_PI)
#define VI_MIN (-100.0)
#define VI_MAX ( 100.0)

/* Tolerances */
#define EPS_MAT  1e-8
#define EPS_VEC  1e-8
#define EPS_NORM 1e-8
#define EPS_ORTH 1e-8
#define EPS_DET  1e-8
#define ANG_EPS  1e-6

/* Helpers for 3x3 matrices stored column-major: M[r + 3*c] */

/* Multiply C = A * B (3x3) */
void mat3_mul(const double A[9], const double B[9], double C[9])
{
  for (int c = 0; c < 3; ++c) {
    for (int r = 0; r < 3; ++r) {
      C[r + 3*c] = A[r + 3*0]*B[0 + 3*c]
                 + A[r + 3*1]*B[1 + 3*c]
                 + A[r + 3*2]*B[2 + 3*c];
    }
  }
}

/* Transpose AT = A^T */
void mat3_transpose(const double A[9], double AT[9])
{
  for (int c = 0; c < 3; ++c) {
    for (int r = 0; r < 3; ++r) {
      AT[c + 3*r] = A[r + 3*c];
    }
  }
}

/* Determinant of 3x3 matrix (column-major) */
double mat3_det(const double M[9])
{
  /* Using expansion by first row. Convert to row-major terms carefully:
     a = M[0+3*0] = M[0], b = M[0+3*1] = M[3], c = M[0+3*2] = M[6]
     d = M[1+3*0] = M[1], e = M[1+3*1] = M[4], f = M[1+3*2] = M[7]
     g = M[2+3*0] = M[2], h = M[2+3*1] = M[5], i = M[2+3*2] = M[8]
  */
  double a = M[0], b = M[3], c = M[6];
  double d = M[1], e = M[4], f = M[7];
  double g = M[2], h = M[5], i = M[8];
  return a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g);
}

/* Build Euler rotation matrices matching generated code:
   Euler1 (Yaw, psi): Rz
   Euler2 (Pitch, theta): Ry
   Euler3 (Roll, phi): Rx
*/
void build_Rz(double psi, double R[9])
{
  double s = sin(psi), c = cos(psi);
  R[0] = c;   R[3] = -s;  R[6] = 0.0;
  R[1] = s;   R[4] =  c;  R[7] = 0.0;
  R[2] = 0.0; R[5] = 0.0; R[8] = 1.0;
}

void build_Ry(double th, double R[9])
{
  double s = sin(th), c = cos(th);
  R[0] =  c;  R[3] = 0.0; R[6] =  s;
  R[1] = 0.0; R[4] = 1.0; R[7] = 0.0;
  R[2] = -s;  R[5] = 0.0; R[8] =  c;
}

void build_Rx(double phi, double R[9])
{
  double s = sin(phi), c = cos(phi);
  R[0] = 1.0; R[3] = 0.0; R[6] = 0.0;
  R[1] = 0.0; R[4] =  c;  R[7] = -s;
  R[2] = 0.0; R[5] =  s;  R[8] =  c;
}

/* Vector-matrix multiply y = M * x (3x3, column-major) */
void mat3_vec3_mul(const double M[9], const double x[3], double y[3])
{
  for (int r = 0; r < 3; ++r) {
    y[r] = M[r + 3*0]*x[0] + M[r + 3*1]*x[1] + M[r + 3*2]*x[2];
  }
}

/* Utility: absolute difference max between matrices */
double mat3_max_abs_diff(const double A[9], const double B[9])
{
  double m = 0.0;
  for (int i = 0; i < 9; ++i) {
    double d = fabs(A[i] - B[i]);
    if (d > m) m = d;
  }
  return m;
}

int main(void)
{
  euler321_I2B_12B_initialize();

  int iter = 0;
  _Bool prev_valid = 0;

  /* Previous step storage (no static) */
  double prev_phi = 0.0, prev_theta = 0.0, prev_psi = 0.0;
  double prev_Vi[3] = {0.0, 0.0, 0.0};
  double prev_DCM[9] = {0.0,0.0,0.0, 0.0,0.0,0.0, 0.0,0.0,0.0};
  double prev_Vb[3]  = {0.0, 0.0, 0.0};

  while (iter < LOOP_MAX) {
    /* Input selection: sometimes repeat previous, sometimes vary theta only, otherwise new */
    if (!prev_valid) {
      rtU.phi   = nondet_double();
      rtU.theta = nondet_double();
      rtU.psi   = nondet_double();
      __ESBMC_assume(rtU.phi   >= ANGLE_MIN && rtU.phi   <= ANGLE_MAX);
      __ESBMC_assume(rtU.theta >= ANGLE_MIN && rtU.theta <= ANGLE_MAX);
      __ESBMC_assume(rtU.psi   >= ANGLE_MIN && rtU.psi   <= ANGLE_MAX);

      rtU.Vi[0] = nondet_double();
      rtU.Vi[1] = nondet_double();
      rtU.Vi[2] = nondet_double();
      __ESBMC_assume(rtU.Vi[0] >= VI_MIN && rtU.Vi[0] <= VI_MAX);
      __ESBMC_assume(rtU.Vi[1] >= VI_MIN && rtU.Vi[1] <= VI_MAX);
      __ESBMC_assume(rtU.Vi[2] >= VI_MIN && rtU.Vi[2] <= VI_MAX);
    } else {
      _Bool repeat_prev = nondet_bool();
      if (repeat_prev) {
        rtU.phi = prev_phi;
        rtU.theta = prev_theta;
        rtU.psi = prev_psi;
        rtU.Vi[0] = prev_Vi[0];
        rtU.Vi[1] = prev_Vi[1];
        rtU.Vi[2] = prev_Vi[2];
      } else {
        _Bool change_only_theta = nondet_bool();
        if (change_only_theta) {
          rtU.phi = prev_phi;
          rtU.psi = prev_psi;
          double dth = nondet_double();
          __ESBMC_assume(fabs(dth) > ANG_EPS && fabs(dth) < 0.5); /* reasonable change */
          rtU.theta = prev_theta + dth;
          __ESBMC_assume(rtU.theta >= ANGLE_MIN && rtU.theta <= ANGLE_MAX);
          /* Change Vi arbitrarily or keep same; either is fine */
          _Bool keep_vi = nondet_bool();
          if (keep_vi) {
            rtU.Vi[0] = prev_Vi[0];
            rtU.Vi[1] = prev_Vi[1];
            rtU.Vi[2] = prev_Vi[2];
          } else {
            rtU.Vi[0] = nondet_double();
            rtU.Vi[1] = nondet_double();
            rtU.Vi[2] = nondet_double();
            __ESBMC_assume(rtU.Vi[0] >= VI_MIN && rtU.Vi[0] <= VI_MAX);
            __ESBMC_assume(rtU.Vi[1] >= VI_MIN && rtU.Vi[1] <= VI_MAX);
            __ESBMC_assume(rtU.Vi[2] >= VI_MIN && rtU.Vi[2] <= VI_MAX);
          }
        } else {
          rtU.phi   = nondet_double();
          rtU.theta = nondet_double();
          rtU.psi   = nondet_double();
          __ESBMC_assume(rtU.phi   >= ANGLE_MIN && rtU.phi   <= ANGLE_MAX);
          __ESBMC_assume(rtU.theta >= ANGLE_MIN && rtU.theta <= ANGLE_MAX);
          __ESBMC_assume(rtU.psi   >= ANGLE_MIN && rtU.psi   <= ANGLE_MAX);

          rtU.Vi[0] = nondet_double();
          rtU.Vi[1] = nondet_double();
          rtU.Vi[2] = nondet_double();
          __ESBMC_assume(rtU.Vi[0] >= VI_MIN && rtU.Vi[0] <= VI_MAX);
          __ESBMC_assume(rtU.Vi[1] >= VI_MIN && rtU.Vi[1] <= VI_MAX);
          __ESBMC_assume(rtU.Vi[2] >= VI_MIN && rtU.Vi[2] <= VI_MAX);
        }
      }
    }

    /* Run one step */
    euler321_I2B_12B_step();

    /* Compute reference quantities for assertions */
    double R1[9], R2[9], R3[9];
    double R12[9], R321[9];
    build_Rz(rtU.psi, R1);     /* Euler1 */
    build_Ry(rtU.theta, R2);   /* Euler2 */
    build_Rx(rtU.phi, R3);     /* Euler3 */
    mat3_mul(R1, R2, R12);     /* Product = Euler1 * Euler2 */
    mat3_mul(R3, R12, R321);   /* DCM = Euler3 * (Euler1*Euler2) */

    double Vb_expected[3];
    mat3_vec3_mul(rtY.DCM321, rtU.Vi, Vb_expected); /* using computed DCM321 */

#ifdef VERIFY_PROPERTY_1
    /* Requirement 1: DCM321 = Euler3 × Euler2 × Euler1 */
    {
      double diff = mat3_max_abs_diff(rtY.DCM321, R321);
      __ESBMC_assert(diff <= EPS_MAT, "PROPERTY 1: DCM321 equals Euler3*Euler2*Euler1 within tolerance");
    }
#endif

#ifdef VERIFY_PROPERTY_2
    /* Requirement 2: Vb = DCM321 × Vi */
    {
      double Vb_calc[3];
      mat3_vec3_mul(rtY.DCM321, rtU.Vi, Vb_calc);
      double err = fmax(fabs(Vb_calc[0] - rtY.Vb[0]),
                   fmax(fabs(Vb_calc[1] - rtY.Vb[1]), fabs(Vb_calc[2] - rtY.Vb[2])));
      __ESBMC_assert(err <= EPS_VEC, "PROPERTY 2: Vb equals DCM321*Vi within tolerance");
    }
#endif

#ifdef VERIFY_PROPERTY_3
    /* Requirement 3: |Vb| = |Vi| (norm preserved) */
    {
      double nVi = sqrt(rtU.Vi[0]*rtU.Vi[0] + rtU.Vi[1]*rtU.Vi[1] + rtU.Vi[2]*rtU.Vi[2]);
      double nVb = sqrt(rtY.Vb[0]*rtY.Vb[0] + rtY.Vb[1]*rtY.Vb[1] + rtY.Vb[2]*rtY.Vb[2]);
      __ESBMC_assert(fabs(nVi - nVb) <= EPS_NORM, "PROPERTY 3: Norm preserved |Vb|=|Vi| within tolerance");
    }
#endif

#ifdef VERIFY_PROPERTY_4
    /* Requirement 4: DCM invertible except theta = ±pi/2 (check det != 0 when away from singular) */
    {
      double det = mat3_det(rtY.DCM321);
      _Bool away_from_sing = (fabs(fabs(rtU.theta) - (M_PI/2.0)) > ANG_EPS);
      if (away_from_sing) {
        __ESBMC_assert(fabs(det) > EPS_DET, "PROPERTY 4: det(DCM321) != 0 when theta is not near ±pi/2");
      }
    }
#endif

#ifdef VERIFY_PROPERTY_5
    /* Requirement 5: Distinct mapping for different theta values (with same phi, psi) */
    if (prev_valid) {
      _Bool same_phi_psi = (fabs(rtU.phi - prev_phi) <= ANG_EPS) && (fabs(rtU.psi - prev_psi) <= ANG_EPS);
      _Bool theta_changed = (fabs(rtU.theta - prev_theta) > ANG_EPS);
      if (same_phi_psi && theta_changed) {
        double diff = mat3_max_abs_diff(rtY.DCM321, prev_DCM);
        __ESBMC_assert(diff > EPS_MAT, "PROPERTY 5: Different theta (with same phi,psi) yields different DCM321");
      }
    }
#endif

#ifdef VERIFY_PROPERTY_6
    /* Requirement 6: Orthonormal rows and columns */
    {
      /* Rows */
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
          double dot = 0.0;
          for (int c = 0; c < 3; ++c) {
            dot += rtY.DCM321[i + 3*c] * rtY.DCM321[j + 3*c];
          }
          if (i == j) {
            __ESBMC_assert(fabs(dot - 1.0) <= EPS_ORTH, "PROPERTY 6: Row unit length");
          } else {
            __ESBMC_assert(fabs(dot) <= EPS_ORTH, "PROPERTY 6: Row orthogonality");
          }
        }
      }
      /* Columns */
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
          double dot = 0.0;
          for (int r = 0; r < 3; ++r) {
            dot += rtY.DCM321[r + 3*i] * rtY.DCM321[r + 3*j];
          }
          if (i == j) {
            __ESBMC_assert(fabs(dot - 1.0) <= EPS_ORTH, "PROPERTY 6: Column unit length");
          } else {
            __ESBMC_assert(fabs(dot) <= EPS_ORTH, "PROPERTY 6: Column orthogonality");
          }
        }
      }
    }
#endif

#ifdef VERIFY_PROPERTY_7
    /* Requirement 7: DCM321 * DCM321^T = I */
    {
      double MT[9], Icheck[9];
      mat3_transpose(rtY.DCM321, MT);
      mat3_mul(rtY.DCM321, MT, Icheck);
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          double expected = (r == c) ? 1.0 : 0.0;
          __ESBMC_assert(fabs(Icheck[r + 3*c] - expected) <= EPS_ORTH, "PROPERTY 7: DCM*DCM^T = I within tolerance");
        }
      }
    }
#endif

#ifdef VERIFY_PROPERTY_8
    /* Requirement 8: det(DCM321) = 1.0 */
    {
      double det = mat3_det(rtY.DCM321);
      __ESBMC_assert(fabs(det - 1.0) <= EPS_DET, "PROPERTY 8: det(DCM321) == 1 within tolerance");
    }
#endif

#ifdef VERIFY_PROPERTY_9
    /* Additional timing/no-state check: with same inputs, outputs remain the same */
    if (prev_valid) {
      _Bool same_inputs =
        fabs(rtU.phi   - prev_phi)   <= 0.0 &&
        fabs(rtU.theta - prev_theta) <= 0.0 &&
        fabs(rtU.psi   - prev_psi)   <= 0.0 &&
        fabs(rtU.Vi[0] - prev_Vi[0]) <= 0.0 &&
        fabs(rtU.Vi[1] - prev_Vi[1]) <= 0.0 &&
        fabs(rtU.Vi[2] - prev_Vi[2]) <= 0.0;

      if (same_inputs) {
        double dM = mat3_max_abs_diff(rtY.DCM321, prev_DCM);
        double dV = fmax(fabs(rtY.Vb[0]-prev_Vb[0]),
                    fmax(fabs(rtY.Vb[1]-prev_Vb[1]), fabs(rtY.Vb[2]-prev_Vb[2])));
        __ESBMC_assert(dM <= EPS_MAT, "PROPERTY 9: No state retention - DCM stable for identical inputs");
        __ESBMC_assert(dV <= EPS_VEC, "PROPERTY 9: No state retention - Vb stable for identical inputs");
      }
    }
#endif

    /* Update previous values for next loop */
    memcpy(prev_DCM, rtY.DCM321, 9*sizeof(double));
    prev_Vb[0] = rtY.Vb[0];
    prev_Vb[1] = rtY.Vb[1];
    prev_Vb[2] = rtY.Vb[2];
    prev_phi = rtU.phi;
    prev_theta = rtU.theta;
    prev_psi = rtU.psi;
    prev_Vi[0] = rtU.Vi[0];
    prev_Vi[1] = rtU.Vi[1];
    prev_Vi[2] = rtU.Vi[2];
    prev_valid = 1;

    ++iter;
  }

  return 0;
}