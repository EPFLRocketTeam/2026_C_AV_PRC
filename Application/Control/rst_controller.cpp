#include "Application/Control/rst_controller.hpp"

#include <cstddef>

namespace prc {

const float kRstDesiredPole[kRstNP_V] = {1.0f, -1.969701875673162f, 0.970057432960381f}; // [1.9, 0.8] wn, damp

namespace {

// For-computation orders, matching BVDPR_lib.h's NA_p/NB_p/NR_p/NS_p/NM.
constexpr int kNA_p = kRstNA + kRstNHS;      // 2
constexpr int kNB_p = kRstNB + kRstNHR;      // 2
constexpr int kNR_p = kRstNA + kRstNHS - 1;  // 1
constexpr int kNS_p = kRstNB + kRstNHR - 1;  // 1
constexpr int kNM   = kNA_p + kNB_p;         // 4

// Gain-scheduling model params, ported from BVDPR_lib.h's model_params[] --
// currently unused (see computeModelB below), kept for documentation parity
// with the source.
constexpr float kModelParams[3] = {1.477714824969535e3f, 0.010362108286342f, -5.875853844056950e2f};

void ComputeModelB(float p_copv_bar, float b[kRstNB_V]) {
  (void)p_copv_bar;
  (void)kModelParams;
  b[0] = 0.0f;
  // b[1] = kModelParams[0] + expf(kModelParams[1] * (p_copv_bar - kModelParams[2]));
  b[1] = 1.2e4f; // gain scheduling disabled, ported as-is from BVDPR_lib.cpp
}

// Polynomial convolution -- direct port of BVDPR_lib.cpp's conv().
void Conv(const float *a, int a_size, const float *b, int b_size, float *r, int r_size) {
  for (int i = 0; i < r_size; ++i) r[i] = 0.0f;
  for (int i = 0; i < a_size; ++i)
    for (int j = 0; j < b_size; ++j)
      r[i + j] += a[i] * b[j];
}

// Solves m * x = p for x via Gauss-Jordan elimination with partial
// pivoting, replacing BLA::Invert(m) + (m * p) from BVDPR_lib.cpp's
// updateRST() -- BDPR inverts m and then multiplies by p; here we solve the
// system directly instead of forming the inverse, which is numerically
// equivalent for a single right-hand side and avoids needing a general
// matrix-inverse routine for a single-use 4x4 solve. Returns false if m is
// singular (mirrors BLA::Invert's `invertible` return).
bool SolveLinearSystem(float m[kNM][kNM], const float p[kNM], float x[kNM]) {
  float a[kNM][kNM + 1];
  for (int i = 0; i < kNM; ++i) {
    for (int j = 0; j < kNM; ++j) a[i][j] = m[i][j];
    a[i][kNM] = p[i];
  }

  for (int col = 0; col < kNM; ++col) {
    int pivot_row = col;
    float pivot_val = a[col][col] < 0 ? -a[col][col] : a[col][col];
    for (int row = col + 1; row < kNM; ++row) {
      float v = a[row][col] < 0 ? -a[row][col] : a[row][col];
      if (v > pivot_val) { pivot_val = v; pivot_row = row; }
    }
    if (pivot_val < 1e-12f) return false;

    if (pivot_row != col) {
      for (int j = 0; j <= kNM; ++j) {
        float tmp = a[col][j];
        a[col][j] = a[pivot_row][j];
        a[pivot_row][j] = tmp;
      }
    }

    float pivot = a[col][col];
    for (int j = 0; j <= kNM; ++j) a[col][j] /= pivot;

    for (int row = 0; row < kNM; ++row) {
      if (row == col) continue;
      float factor = a[row][col];
      if (factor == 0.0f) continue;
      for (int j = 0; j <= kNM; ++j) a[row][j] -= factor * a[col][j];
    }
  }

  for (int i = 0; i < kNM; ++i) x[i] = a[i][kNM];
  return true;
}

} // namespace

bool UpdateRstPolynomials(float r[kRstNR_V], float s[kRstNS_V], float t[kRstNT_V], float p_copv_bar) {
  float a_in[kRstNA + 1] = {1.0f, -0.9784f};
  float b_in[kRstNB_V];
  ComputeModelB(p_copv_bar, b_in);

  // Hs and Hr are fixed -- from BVDPR_lib.cpp's updateRST().
  float Hs[kRstNHS + 1] = {1.0f, -1.0f};
  float Hr[kRstNHR + 1] = {1.0f, 1.0f};

  float a[kNA_p + 1];
  Conv(a_in, kRstNA + 1, Hs, kRstNHS + 1, a, kNA_p + 1);

  float b[kNB_p + 1];
  Conv(b_in, kRstNB_V, Hr, kRstNHR + 1, b, kNB_p + 1);

  // Build system matrix M -- same construction as BVDPR_lib.cpp's updateRST().
  float m[kNM][kNM] = {{0}};

  for (int i = 0; i < kNB_p; ++i)
    for (int j = 0; j <= kNA_p; ++j)
      m[j + i][i] = a[j];

  for (int i = 0; i < kNA_p; ++i)
    for (int j = 0; j <= kNB_p; ++j)
      m[j + i][kNB_p + i] = b[j];

  // Desired closed-loop pole vector, zero-padded to kNM rows.
  float p_vec[kNM];
  for (int i = 0; i < kNM; ++i)
    p_vec[i] = (i <= kRstNP) ? kRstDesiredPole[i] : 0.0f;

  float coeffs[kNM];
  if (!SolveLinearSystem(m, p_vec, coeffs)) return false;

  float r_p[kNR_p + 1];
  for (int i = 0; i <= kNR_p; ++i) r_p[i] = coeffs[kNS_p + 1 + i];

  float s_p[kNS_p + 1];
  for (int i = 0; i <= kNS_p; ++i) s_p[i] = coeffs[i];

  // Convolve S and R with Hs and Hr.
  Conv(r_p, kNR_p + 1, Hr, kRstNHR + 1, r, kRstNR + 1);
  Conv(s_p, kNS_p + 1, Hs, kRstNHS + 1, s, kRstNS + 1);

  t[0] = 0.0f;
  for (int i = 0; i <= kRstNR; ++i) t[0] += r[i];

  return true;
}

RstController::RstController(const float *r, const float *s, const float *t) : r_(r), s_(s), t_(t) {}

void RstController::reset(float current_measurement, float u0) {
  for (float &v : d_y_)   v = current_measurement;
  for (float &v : d_ref_) v = current_measurement;
  for (float &v : d_u_)   v = u0;
}

float RstController::update(float reference, float measurement) {
  // Shift history right, insert new sample at index 0 (front) -- replaces
  // std::deque's push_front()/pop_back() with manual array shifting.
  for (int i = kRstNT_V - 1; i > 0; --i) d_ref_[i] = d_ref_[i - 1];
  d_ref_[0] = reference;

  for (int i = kRstNS_V - 1; i > 0; --i) d_y_[i] = d_y_[i - 1];
  d_y_[0] = measurement;

  float control = 0.0f;

  for (int i = 0; i < kRstNT_V; ++i) control += t_[i] * d_ref_[i];

  for (int i = 1; i < kRstNS_V; ++i) control -= s_[i] * d_u_[i - 1];

  for (int i = 0; i < kRstNR_V; ++i) control -= r_[i] * d_y_[i];

  for (int i = kRstNR_V - 1; i > 0; --i) d_u_[i] = d_u_[i - 1];
  d_u_[0] = control;

  return control;
}

// Bench-characterized flow/angle table -- verbatim from BVDPR_lib.cpp's
// theta_data/phi_data (fixed arrays here instead of std::vector).
namespace {
constexpr int kFlowTablePoints = 21;
constexpr float kThetaData[kFlowTablePoints] = {
    15.0f, 15.5f, 15.75f, 16.0f, 16.5f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f,
    23.0f, 25.0f, 30.0f,  35.0f, 40.0f, 45.0f, 50.0f, 60.0f, 65.0f, 70.0f};
constexpr float kPhiData[kFlowTablePoints] = {
    7.675455e-13f, 9.525601e-10f, 1.437586e-08f, 3.164209e-08f, 7.474652e-08f,
    1.465754e-07f, 3.160894e-07f, 5.314604e-07f, 7.790231e-07f, 1.073049e-06f,
    1.336290e-06f, 1.692977e-06f, 2.298087e-06f, 3.796715e-06f, 5.017496e-06f,
    5.759568e-06f, 6.192016e-06f, 6.401845e-06f, 6.502824e-06f, 6.613122e-06f,
    6.632663e-06f};
} // namespace

float FlowToAngleDeg(float phi) {
  if (phi < kPhiData[0]) return kServoMinAngleDeg;
  if (phi > kPhiData[kFlowTablePoints - 1]) return kServoMaxAngleDeg;

  for (int i = 1; i < kFlowTablePoints; ++i) {
    if (phi <= kPhiData[i]) {
      float phi1 = kPhiData[i - 1];
      float phi2 = kPhiData[i];
      float theta1 = kThetaData[i - 1];
      float theta2 = kThetaData[i];
      return theta1 + (phi - phi1) * (theta2 - theta1) / (phi2 - phi1);
    }
  }

  return kServoMinAngleDeg; // failsafe, mirrors BVDPR_lib.cpp's flow2angle()
}

float AngleDegToPercentOpen(float angle_deg) {
  if (angle_deg < kServoMinAngleDeg) angle_deg = kServoMinAngleDeg;
  if (angle_deg > kServoMaxAngleDeg) angle_deg = kServoMaxAngleDeg;
  return (angle_deg / kServoMaxAngleDeg) * 100.0f;
}

} // namespace prc
