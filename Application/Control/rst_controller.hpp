//    FILE: rst_controller.hpp
// PURPOSE: RST (pole-placement) pressure controller + flow->servo-angle
//          characterization, ported from 2026_C_PR_BDPR's BVDPR_lib.h/.cpp
//          -- the bench-validated ball-valve DPR firmware for this vehicle.
//          Replaces prc_state.cpp's earlier placeholder linear proportional
//          law (percent_open = 50 + gain*error).
//
// This is a faithful port of BDPR's actual control law -- same plant model,
// same fixed Hs/Hr terms, same desired-pole polynomial (tuned for wn=1.9,
// damp=0.8 via BDPR's rst_test.m), same Diophantine-equation solve, same
// flow/angle characterization table -- only the implementation substrate
// changed, since the original depends on two Arduino-only pieces this
// STM32 HAL/bare-metal C++ project doesn't have:
//   - BasicLinearAlgebra (BLA::Matrix + Invert()) for the one (small, fixed
//     4x4) matrix inversion in UpdateRstPolynomials() -- reimplemented here
//     as a self-contained Gauss-Jordan solve.
//   - std::deque<float> for the controller's sliding input/output history
//     -- reimplemented here with fixed-size arrays and manual shifting,
//     since every history length (NR_V=3, NS_V=3, NT_V=1) is a compile-time
//     constant; heap-allocating deques are unnecessary overhead for
//     1-3 element buffers in flight firmware.
//
// NOTE: BDPR's gain scheduling (adjusting the plant model B coefficient
// from live COPV pressure) is disabled upstream in BVDPR_lib.cpp itself
// (computeModelB() hardcodes b[1]=1.2e4 regardless of its p_COPV argument,
// per BDPR's own "Cste to disable gain scheduling" comment) -- ported
// as-is, disabled, not something lost in translation here.

#ifndef APP_PRC_RST_CONTROLLER_H
#define APP_PRC_RST_CONTROLLER_H

namespace prc {

// ---------------------------------------------------------------------------
// RST controller structure -- from BVDPR_lib.h (NA=NB=NHR=NHS=1, NP=2).
// ---------------------------------------------------------------------------

constexpr int kRstNA  = 1; // plant denominator order
constexpr int kRstNB  = 1; // plant numerator order
constexpr int kRstNHR = 1; // fixed term order
constexpr int kRstNHS = 1; // fixed term order

constexpr int kRstNA_V = kRstNA + 1; // 2
constexpr int kRstNB_V = kRstNB + 1; // 2

constexpr int kRstNR  = kRstNA + kRstNHS + kRstNHR - 1; // 2
constexpr int kRstNS  = kRstNB + kRstNHS + kRstNHR - 1; // 2
constexpr int kRstNT  = 0;                              // same tracking/regulation performance
constexpr int kRstNP  = 2;                              // desired closed-loop pole order

constexpr int kRstNR_V = kRstNR + 1; // 3
constexpr int kRstNS_V = kRstNS + 1; // 3
constexpr int kRstNT_V = kRstNT + 1; // 1
constexpr int kRstNP_V = kRstNP + 1; // 3

// Desired closed-loop pole polynomial P_d -- from BDPR's rst_test.m,
// currently tuned for [wn=1.9 rad/s, damp=0.8] (the selected line in
// BVDPR_lib.h; other tunings are commented out there).
extern const float kRstDesiredPole[kRstNP_V];

// Computes this tick's R/S/T controller polynomials via pole placement
// (Diophantine equation solve). p_copv_bar is accepted for interface
// parity with BDPR's updateRST() but currently has no effect -- see the
// gain-scheduling note above. Returns false (leaving r/s/t unchanged) if
// the internal system matrix is singular.
bool UpdateRstPolynomials(float r[kRstNR_V], float s[kRstNS_V], float t[kRstNT_V], float p_copv_bar);

// ---------------------------------------------------------------------------
// RstController -- the actual discrete controller, ported from BDPR's
// RSTController class. Two independent instances are needed (matching
// BDPR's RST_p / RST_c): one for the open-loop pressurization ramp, one for
// closed-loop regulation, each with its own R/S/T polynomials and history.
// ---------------------------------------------------------------------------

class RstController {
public:
  // r/s/t must outlive this controller -- same non-owning-pointer design as
  // BDPR's original (they're computed once by UpdateRstPolynomials() into
  // caller-owned storage, not copied).
  RstController(const float *r, const float *s, const float *t);

  // Resets the controller's history to a steady value. Call once on entry
  // to the state that owns this controller instance (matches BDPR calling
  // RST_p.set()/RST_c.set() once per FSM-state entry, not every tick).
  void reset(float current_measurement, float u0 = 0.0f);

  // Computes and returns the next control action for this tick.
  float update(float reference, float measurement);

private:
  const float *r_;
  const float *s_;
  const float *t_;

  // Index 0 = most recent. Sized to match BDPR's d_u/d_y/d_ref deques.
  float d_u_[kRstNR_V]   = {0};
  float d_y_[kRstNS_V]   = {0};
  float d_ref_[kRstNT_V] = {0};
};

// ---------------------------------------------------------------------------
// Valve characterization (flow -> servo angle) -- from BVDPR_lib.cpp/.h.
// ---------------------------------------------------------------------------

constexpr float kServoMinAngleDeg  = 0.0f;
constexpr float kServoMaxAngleDeg  = 70.0f;
constexpr float kServoZeroAngleDeg = 16.0f; // below this, characterized flow is ~0 (dead-band)

// Converts the RST controller's raw output ("phi", a flow-proxy value) to
// a servo angle in degrees, via linear interpolation over BDPR's
// bench-characterized flow/angle curve. Clamps to
// [kServoMinAngleDeg, kServoMaxAngleDeg] outside the characterized range.
float FlowToAngleDeg(float phi);

// Converts a servo angle to the 0..100 percent-open this project's
// ServoBallValve::set_position() expects (Drivers/Valve/Valve.hpp) -- that
// driver's abstraction is percent-of-travel between two calibrated PWM
// endpoints, not a raw angle/pulse-width like BDPR's direct
// ANGLE_TO_WIDTH() mapping, so this conversion is this project's addition,
// not a ported BDPR function.
float AngleDegToPercentOpen(float angle_deg);

} // namespace prc

#endif // APP_PRC_RST_CONTROLLER_H
