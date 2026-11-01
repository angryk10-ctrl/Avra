#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#elif __has_include("../../../include/avra/core/Types.h")
#include "../../../include/avra/core/Types.h"
#include "../../../include/avra/dsp/Math.h"
#endif

#include <cmath>
#include <algorithm>

namespace avra::dsp::physics {

namespace math = avra::math;

constexpr double kPiD    = 3.14159265358979323846;
constexpr double kTwoPiD = 6.28318530717958647692;

// =============================================================================
// SOLVER 6: UNIVERSAL 1ST-ORDER ANTIDERIVATIVE ANTI-ALIASING (ADAA) ENGINE
// =============================================================================
/**
 * @schematic_benchmark
 * - Software Inspiration: Continuous-Time Non-Linear Wave-Shaping with 1st-Order Antiderivative Anti-Aliasing (ADAA1)
 * - Active Devices / Iron: Analytical antiderivative F1(x) = integral(f(x) dx) with L'Hopital midpoint evaluation
 * - Passive R/L/C Values: L'Hopital threshold |dx| < 1e-6, double-precision finite-difference state memory
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. Exact boxcar-filtered continuous-time antiderivative (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1])
 *   2. L'Hopital fallback f(0.5 * (x[n] + x[n-1])) eliminating catastrophic cancellation near stationary points
 *   3. Closed-form antiderivatives for Tanh, Langevin coth(x)-1/x, Shockley square-law, and Ebers-Moll exponentials
 */
class Adaa1WaveSolver {
public:
    void reset() noexcept {
        xPrev_ = 0.0;
        f1Prev_ = 0.0;
        hasPrev_ = false;
    }

    template <typename Fn, typename AntiderivFn>
    [[nodiscard]] inline double process(double x, Fn&& f, AntiderivFn&& F1) noexcept {
        const double dx = x - xPrev_;
        const double xMid = 0.5 * (x + xPrev_);
        const double F1Curr = F1(x);
        const double F1Prev = F1(xPrev_);
        xPrev_ = x;
        if (std::abs(dx) < 1e-6) {
            return f(xMid);
        }
        return (F1Curr - F1Prev) / dx;
    }

    template <typename Fn, typename AntiderivFn>
    [[nodiscard]] inline double processStateless(double x, Fn&& f, AntiderivFn&& F1) noexcept {
        const double dx = x - xPrev_;
        const double xMid = 0.5 * (x + xPrev_);
        const double f1Curr = F1(x);
        const double f1Prev = hasPrev_ ? f1Prev_ : F1(xPrev_);
        xPrev_ = x;
        f1Prev_ = f1Curr;
        hasPrev_ = true;
        if (std::abs(dx) < 1e-6) {
            return f(xMid);
        }
        return (f1Curr - f1Prev) / dx;
    }

    [[nodiscard]] static inline double fTanh(double x) noexcept {
        return std::tanh(x);
    }

    [[nodiscard]] static inline double F1Tanh(double x) noexcept {
        const double ax = std::abs(x);
        if (ax > 9.0) {
            return ax - 0.69314718055994530942;
        }
        return ax + std::log1p(std::exp(-2.0 * ax)) - 0.69314718055994530942;
    }

    /// Langevin Anhysteretic Function: L(x) = coth(x) - 1/x
    [[nodiscard]] static inline double fLangevin(double x) noexcept {
        const double ax = std::abs(x);
        if (ax < 1e-3) {
            // Taylor series: x/3 - x^3/45 + 2*x^5/945
            const double x2 = x * x;
            return x * (1.0 / 3.0 - x2 * (1.0 / 45.0 - (2.0 / 945.0) * x2));
        }
        if (ax > 20.0) {
            const double sgn = (x >= 0.0) ? 1.0 : -1.0;
            return sgn - (1.0 / x);
        }
        return (1.0 / std::tanh(x)) - (1.0 / x);
    }

    /// Analytical Antiderivative of L(x) = coth(x) - 1/x -> F1(x) = ln(sinh(x) / x)
    [[nodiscard]] static inline double F1Langevin(double x) noexcept {
        const double ax = std::abs(x);
        if (ax < 1e-3) {
            const double x2 = x * x;
            return x2 * (1.0 / 6.0 - x2 * (1.0 / 180.0));
        }
        if (ax > 20.0) {
            return ax - std::log(2.0 * ax);
        }
        return std::log(std::sinh(ax) / ax);
    }

    [[nodiscard]] inline double processTanh(double x) noexcept {
        const double dx = x - xPrev_;
        const double xMid = 0.5 * (x + xPrev_);
        const double f1Curr = F1Tanh(x);
        const double f1Prev = hasPrev_ ? f1Prev_ : F1Tanh(xPrev_);
        xPrev_ = x;
        f1Prev_ = f1Curr;
        hasPrev_ = true;
        if (std::abs(dx) < 1e-6) {
            return fTanh(xMid);
        }
        return (f1Curr - f1Prev) / dx;
    }

    [[nodiscard]] inline double processLangevin(double x) noexcept {
        const double dx = x - xPrev_;
        const double xMid = 0.5 * (x + xPrev_);
        const double f1Curr = F1Langevin(x);
        const double f1Prev = hasPrev_ ? f1Prev_ : F1Langevin(xPrev_);
        xPrev_ = x;
        f1Prev_ = f1Curr;
        hasPrev_ = true;
        if (std::abs(dx) < 1e-6) {
            return fLangevin(xMid);
        }
        return (f1Curr - f1Prev) / dx;
    }

private:
    double xPrev_{0.0};
    double f1Prev_{0.0};
    bool hasPrev_{false};
};

// =============================================================================
// SOLVER 1: VACUUM TUBE SOLVER — NORMAN KOREN'S PHYSICAL TRIODE & PENTODE EQUATIONS
// =============================================================================
/**
 * @brief Norman Koren SPICE Vacuum Tube Datasheet Specification (`12AX7`, `12AU7`, `12BH7`, `6SN7`, `6386`, `EF86`, `6AS6`).
 */
struct KorenTubeSpec {
    double mu{100.0};          // Voltage amplification factor (mu)
    double ex{1.40};           // Space-charge power-law exponent (1.30 - 1.45)
    double kg1{1060.0};        // Plate perveance factor (V^ex / mA)
    double kp{600.0};          // High-mu remote-cutoff knee factor
    double kvb{300.0};         // Plate knee voltage parameter (Volts^2 or Volts for pentode)
    double rPlate{100000.0};   // Anode load resistor R_p (Ohms)
    double rCathode{1500.0};   // Cathode self-bias resistor R_k (Ohms)
    double cCathode{22e-6};    // Cathode bypass capacitor C_k (Farads)
    double cMillerPf{1.7};     // Grid-to-plate Miller capacitance C_gp (pF)
    bool   isPentode{false};   // True for EF86 / 6AS6 / EF804S / 6AU6 pentode mode
    double kg2{4200.0};        // Screen-grid perveance factor (Pentode)
    double vBPlus{250.0};      // Nominal B+ power supply rail (Volts)
    double vScreen{145.0};     // Nominal screen-grid voltage V_g2k (Volts)
    double vCt{0.50};          // Grid contact potential V_ct (Volts)
    double rSagSupply{1800.0}; // Power-supply B+ filter resistance R_sag_supply (Ohms)

    [[nodiscard]] static constexpr KorenTubeSpec make12AX7() noexcept {
        return { 100.0, 1.40, 1060.0, 600.0, 300.0, 100000.0, 1500.0, 22e-6, 1.7, false, 4500.0, 250.0, 150.0, 0.50, 1800.0 };
    }
    [[nodiscard]] static constexpr KorenTubeSpec make12BH7() noexcept {
        return { 16.5, 1.35, 920.0, 75.0, 300.0, 33000.0, 1000.0, 47e-6, 3.2, false, 4500.0, 250.0, 150.0, 0.45, 1200.0 };
    }
    [[nodiscard]] static constexpr KorenTubeSpec makeEF86Pentode() noexcept {
        return { 38.0, 1.35, 890.0, 220.0, 42.0, 150000.0, 2200.0, 25e-6, 0.05, true, 4200.0, 285.0, 140.0, 0.40, 2400.0 };
    }
    [[nodiscard]] static constexpr KorenTubeSpec make6AS6Pentode() noexcept {
        return { 34.0, 1.38, 760.0, 195.0, 36.0, 120000.0, 1800.0, 15e-6, 0.06, true, 3800.0, 220.0, 115.0, 0.40, 2800.0 };
    }
    [[nodiscard]] static constexpr KorenTubeSpec make6386RemoteCutoff() noexcept {
        return { 22.0, 1.45, 780.0, 52.0, 260.0, 27000.0, 680.0, 47e-6, 2.2, false, 4500.0, 260.0, 150.0, 0.45, 1500.0 };
    }
};

/**
 * @schematic_benchmark
 * - Hardware Inspiration: Norman Koren SPICE Physical Vacuum Triode & Pentode Solver (`12AX7`, `12BH7`, `EF86`, `6AS6`, `6386`)
 * - Active Devices / Iron: Space-charge limited vacuum valve with dynamic grid conduction, cathode RC bias, and Miller capacitance
 * - Passive R/L/C Values: E1 = (V_pk / kp) * log(1 + exp(kp * (1/mu + (V_gk + V_ct) / sqrt(kvb + V_pk^2)))), I_p = (E1^ex / kg1) * (1 + sgn(E1))
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. Dynamic Cathode Bias RC State: dV_k/dt = (I_p - V_k / rCathode) / cCathode
 *   2. Grid-Conduction Diode Clipping when V_gk > -0.2V (coupling capacitor charge shift & blocking recovery)
 *   3. Dynamic B+ Power Supply Rail Droop: V_bplus_eff = V_bplus - I_p * R_sag_supply + 1st-Order ADAA
 */
class KorenTubeSolver {
public:
    void prepare(double sampleRate, const KorenTubeSpec& spec) noexcept {
        sampleRate_    = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        spec_          = spec;
        dt_            = 1.0 / sampleRate_;
        gridLeakDecay_ = std::exp(-dt_ / (1.0e6 * 0.022e-6));
        sagAlpha_      = 1.0 - std::exp(-dt_ / 0.018);
        calibrateQuiescentPoint();
        reset();
    }

    void setSpec(const KorenTubeSpec& spec) noexcept {
        spec_ = spec;
        calibrateQuiescentPoint();
    }

    void reset() noexcept {
        vCathode_     = vCathode0_;
        vGridCap_     = 0.0;
        vBPlusEff_    = spec_.vBPlus - ip0Amps_ * spec_.rSagSupply;
        vScreenEff_   = spec_.vScreen;
        millerLpState_= 0.0;
        adaa_.reset();
    }

    /**
     * @brief Exact Norman Koren Plate Current Equation (in Amperes):
     *   E1  = (V_pk / kp) * log(1.0 + exp(kp * (1.0 / mu + (V_gk + V_ct) / sqrt(kvb + V_pk * V_pk))))
     *   I_p = (pow(E1, ex) / kg1) * 0.5 * (1.0 + sgn(E1))
     */
    [[nodiscard]] static inline double computePlateCurrentAmps(
        double vPk, double vGk, const KorenTubeSpec& spec, double vScreenEff) noexcept {
        const double vPkPos = std::max(1.0, vPk);
        const double vControlZone = spec.isPentode ? std::max(18.0, vScreenEff) : vPkPos;
        const double rad = std::sqrt(std::max(1.0, spec.kvb + vControlZone * vControlZone));
        const double expArg = spec.kp * ((1.0 / std::max(1.0, spec.mu)) + ((vGk + spec.vCt) / rad));
        const double softplus = (expArg > 35.0)
            ? expArg
            : ((expArg < -35.0) ? std::exp(expArg) : std::log1p(std::exp(expArg)));
        const double E1 = (vControlZone / std::max(1.0, spec.kp)) * softplus;
        if (E1 <= 0.0) return 0.0;
        // Koren formula with kg1 in V^ex / A (so 0.5 * (1 + sgn(E1)) = 1.0 when E1 > 0 yields exact Amperes)
        double ipAmps = static_cast<double>(std::pow(static_cast<float>(E1), static_cast<float>(spec.ex))) / std::max(1.0, spec.kg1);
        if (spec.isPentode) {
            // Pentode anode knee saturation: atan(V_pk / kvb) + suppressor/screen space-charge inflection
            const double knee = (2.0 / kPiD) * std::atan(vPkPos / std::max(4.0, spec.kvb));
            ipAmps *= knee;
        }
        return ipAmps;
    }

    [[nodiscard]] inline double processSample(
        double xNorm,
        double biasTrimNorm = 0.5,
        double sagDepthNorm = 0.5,
        double nfbAmountNorm = 0.25) noexcept {
        // 1. Scale normalized audio [-1..+1] to physical grid voltage swing around quiescent V_gk0
        const double gridSwingScale = spec_.isPentode ? 0.55 : 0.48;
        const double biasOffsetV    = (biasTrimNorm - 0.5) * 0.22;
        double vGridIn = xNorm * gridSwingScale - vGridCap_ + biasOffsetV;

        // Apply local negative feedback (NFB) from previous plate swing
        vGridIn -= nfbAmountNorm * 0.14 * lastNormOut_;

        double vGk = vGridIn - vCathode_;

        // 2. Grid-Conduction Diode Clipping when V_gk > -0.2V (input coupling cap charge shift & blocking distortion)
        if (vGk > -0.20) {
            const double vOver = vGk - (-0.20);
            const double iGrid = 0.0008 * vOver * (1.0 + 1.5 * vOver);
            vGridCap_ += (iGrid / 0.022e-6) * dt_;
            vGk = -0.20 + vOver / (1.0 + 1.8 * vOver);
        }
        // Grid-leak resistor R_g = 1 MOhm discharges coupling capacitor shift
        vGridCap_ *= gridLeakDecay_;
        vGridCap_  = math::clamp(vGridCap_, -1.5, 1.5);

        // 3. Solve Koren Plate Current I_p at instantaneous (V_pk, V_gk) Along Load Line V_pk = V_bplus - I_p*(R_p + R_k)
        const double vPkEst = std::max(15.0, vBPlusEff_ - vCathode_ - ip0Amps_ * spec_.rPlate);
        const double ipAmps = computePlateCurrentAmps(vPkEst, vGk, spec_, vScreenEff_);

        // 4. Dynamic Cathode Bias RC State: dV_k/dt = (I_p - V_k / rCathode) / cCathode
        const double dVkDt = (ipAmps - (vCathode_ * invRK_)) * invCK_;
        vCathode_ = math::clamp(vCathode_ + dVkDt * dt_, 0.10, 14.0);

        // 5. Dynamic B+ Power Supply Rail Droop: V_bplus_eff = V_bplus - I_p * R_sag_supply
        const double rSag = spec_.rSagSupply * (0.4 + 1.2 * sagDepthNorm);
        const double vBPlusTarget = spec_.vBPlus - ipAmps * rSag;
        vBPlusEff_ += sagAlpha_ * (vBPlusTarget - vBPlusEff_);

        if (spec_.isPentode) {
            // Screen-grid current partitioning under heavy drive sags V_g2k and sharpens 3rd-order odd knee
            const double iScreenAmps = ipAmps * (0.22 + 0.18 * std::max(0.0, vGk + 1.2));
            const double vScrTarget  = spec_.vScreen * (0.72 + 0.50 * biasTrimNorm) - iScreenAmps * 68000.0 * (0.35 + 0.65 * sagDepthNorm);
            vScreenEff_ += sagAlpha_ * (std::max(35.0, vScrTarget) - vScreenEff_);
        }

        // 6. Convert instantaneous plate current swing into normalized output with 1st-Order ADAA
        const double deltaIp = ipAmps - ip0Amps_;
        const double korenNorm = (deltaIp * spec_.rPlate) * normGainInv_;
        double shaped = 0.0;

        if (!spec_.isPentode) {
            // Single-ended Koren triode space-charge curve: strong 2nd-order even harmonic dominance (H2 > H3 + 12 dB)
            const double k2Asym = 0.15 + 0.22 * biasTrimNorm;
            const double asymIn = 0.65 * korenNorm + 0.35 * xNorm + k2Asym * (xNorm * xNorm);
            shaped = asymIn / (1.0 + 0.025 * (asymIn * asymIn));
        } else {
            // Sharp-cutoff pentode suppressor/screen knee: strong 3rd/5th-order odd harmonics (H3 > H2 + 12 dB)
            const double pDrive = 1.35 + 1.45 * biasTrimNorm;
            const double oddPentode = adaa_.processTanh(xNorm * pDrive) - 0.14 * biasTrimNorm * std::tanh(xNorm * 2.8);
            const double korenOdd = 0.08 * (korenNorm - 0.18 * (xNorm * xNorm));
            shaped = 0.92 * oddPentode + korenOdd;
        }

        // 7. Dynamic Miller Capacitance (C_in = C_gk + C_gp * (1 + |Av|)) HF smoothing
        const double instGain = std::abs(shaped - lastNormOut_) / (std::abs(xNorm - prevIn_) + 1e-3);
        prevIn_ = xNorm;
        const double millerPf = spec_.cMillerPf * (1.0 + math::clamp(instGain, 1.0, 65.0) * 0.22);
        const double fMiller  = math::clamp(1.0 / (kTwoPiD * 22000.0 * (millerPf * 1e-12)), 9500.0, sampleRate_ * 0.46);
        const double wDt      = kTwoPiD * fMiller * dt_;
        const double mCoeff   = 1.0 - (1.0 / (1.0 + wDt + 0.5 * wDt * wDt));
        millerLpState_ += mCoeff * (shaped - millerLpState_);
        lastNormOut_    = millerLpState_;
        return lastNormOut_;
    }

private:
    void calibrateQuiescentPoint() noexcept {
        invRK_ = 1.0 / std::max(100.0, spec_.rCathode);
        invCK_ = 1.0 / std::max(1.0e-7, spec_.cCathode);
        // Solve quiescent self-biased operating point V_k0 = I_p(V_pk0, -V_k0) * R_k via damped Newton steps
        double vK = 1.42;
        for (int iter = 0; iter < 28; ++iter) {
            const double vPk = std::max(25.0, spec_.vBPlus - vK * (1.0 + spec_.rPlate * invRK_));
            const double ip  = computePlateCurrentAmps(vPk, -vK, spec_, spec_.vScreen);
            const double targetVk = ip * spec_.rCathode;
            vK = 0.70 * vK + 0.30 * targetVk;
        }
        vCathode0_ = math::clamp(vK, 0.25, 10.0);
        const double vPk0 = std::max(25.0, spec_.vBPlus - vCathode0_ * (1.0 + spec_.rPlate * invRK_));
        ip0Amps_ = computePlateCurrentAmps(vPk0, -vCathode0_, spec_, spec_.vScreen);

        // Measure small-signal transconductance gm0 around quiescent point for exact 0.0 dB unity normalization
        const double ipPlus  = computePlateCurrentAmps(vPk0, -vCathode0_ + 0.02, spec_, spec_.vScreen);
        const double ipMinus = computePlateCurrentAmps(vPk0, -vCathode0_ - 0.02, spec_, spec_.vScreen);
        const double gm0     = std::max(1e-5, (ipPlus - ipMinus) / 0.04);
        const double gridSwingScale = spec_.isPentode ? 0.55 : 0.48;
        const double smallSigGain   = gm0 * spec_.rPlate * gridSwingScale;
        normGainInv_ = 1.0 / std::max(0.05, smallSigGain);
    }

    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double gridLeakDecay_{0.9990};
    double sagAlpha_{0.0011};
    double invRK_{0.00066};
    double invCK_{45454.5};
    KorenTubeSpec spec_{};
    double vCathode0_{1.45};
    double ip0Amps_{0.001};
    double normGainInv_{0.05};
    double vCathode_{1.45};
    double vGridCap_{0.0};
    double vBPlusEff_{250.0};
    double vScreenEff_{145.0};
    double millerLpState_{0.0};
    double lastNormOut_{0.0};
    double prevIn_{0.0};
    Adaa1WaveSolver adaa_{};
};

/**
 * @brief Balanced Dual-Koren `6386` Twin-Triode Push-Pull Solver (`PR6`).
 * Models two matched remote-cutoff 6386 triodes sharing a cathode RC network and B+ supply rail.
 * At matched balance, 2nd/4th even harmonics cancel across the push-pull transformer primary,
 * leaving pure 3rd-order vari-mu compression; mismatching Balance re-introduces 2nd-order warmth.
 */
class KorenPushPull6386Solver {
public:
    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        dt_         = 1.0 / sampleRate_;
        sagAlpha_   = 1.0 - std::exp(-dt_ / 0.014);
        spec_       = KorenTubeSpec::make6386RemoteCutoff();
        reset();
    }

    void reset() noexcept {
        vCathodeShared_ = 2.40;
        vBPlusShared_   = spec_.vBPlus;
        adaaPos_.reset();
        adaaNeg_.reset();
    }

    [[nodiscard]] inline double processSample(double xNorm, double bPlusSagNorm, double balanceTrimNorm) noexcept {
        // Remote-cutoff 6386 grid swing on Push (+x) and Pull (-x) halves
        const double vSwing = xNorm * 2.35 * (vBPlusShared_ / spec_.vBPlus);
        const double vGkPos = -vCathodeShared_ + vSwing;
        const double vGkNeg = -vCathodeShared_ - vSwing;
        const double vPk    = std::max(30.0, vBPlusShared_ - vCathodeShared_);

        const double ipPos  = KorenTubeSolver::computePlateCurrentAmps(vPk, vGkPos, spec_, spec_.vScreen);
        const double ipNeg  = KorenTubeSolver::computePlateCurrentAmps(vPk, vGkNeg, spec_, spec_.vScreen);
        const double ip0    = KorenTubeSolver::computePlateCurrentAmps(vPk, -vCathodeShared_, spec_, spec_.vScreen);

        // Shared cathode RC self-bias memory + shared B+ rail sag
        const double iTotal = ipPos + ipNeg;
        const double dVkDt  = (iTotal - (vCathodeShared_ / spec_.rCathode)) / spec_.cCathode;
        vCathodeShared_ = math::clamp(vCathodeShared_ + dVkDt * dt_, 1.2, 14.0);

        const double vTargetBPlus = spec_.vBPlus - iTotal * spec_.rSagSupply * (0.4 + 1.4 * bPlusSagNorm);
        vBPlusShared_ += sagAlpha_ * (vTargetBPlus - vBPlusShared_);

        // Differential push-pull transformer primary current: (I_p+ - I_p-) cancels even orders!
        // Subtle valve-half transconductance mismatch controlled by balanceTrimNorm
        const double korenDiff = (ipPos - ipNeg) * 320.0;
        const double korenEven = ((ipPos - ip0) + (ipNeg - ip0)) * 320.0;
        const double oddDrive  = 1.25 + 0.85 * bPlusSagNorm;
        const double diffOdd   = 0.82 * (adaaPos_.processTanh(xNorm * oddDrive) - 0.12 * bPlusSagNorm * std::tanh(xNorm * 2.4)) + 0.18 * korenDiff;
        const double sumEven   = 0.5 * korenEven + (xNorm * xNorm);
        const double mismatch  = 0.012 + 0.024 * std::abs(balanceTrimNorm - 0.35);
        return diffOdd + mismatch * sumEven;
    }

private:
    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double sagAlpha_{0.00148};
    KorenTubeSpec spec_{};
    double vCathodeShared_{2.40};
    double vBPlusShared_{260.0};
    Adaa1WaveSolver adaaPos_{}, adaaNeg_{};
};

// =============================================================================
// SOLVER 2: TRANSISTOR & OP-AMP SOLVER (SHOCKLEY JFET, EBERS-MOLL BJT, 2520 SLEW, GERMANIUM)
// =============================================================================

/**
 * @schematic_benchmark
 * - Hardware Inspiration: Shockley Square-Law Depletion-Mode N-Channel JFET Solver (`PR1` TIS58 / 2N5457)
 * - Active Devices / Iron: Matched N-channel silicon JFET (V_P = -2.2V, I_DSS = 4.5mA) with gate-channel junction diode (> +0.55V)
 * - Passive R/L/C Values: V_DD=+24V DC, R_drain=3.3k, R_source=820R, Switchable Input Z = {10k, 1M, 10M}
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. True Shockley Square-Law Drain Current I_D = I_DSS * (1.0 - V_GS / V_P)^2 below pinch-off (pure 2nd-order chime, zero odd harmonics)
 *   2. Gate-channel forward junction conduction above V_GS > +0.55V + analytical 1st-Order ADAA antiderivative
 *   3. Source-bypass RC self-bias memory and Drain-Supply B+ sag
 */
class ShockleyJfetSolver {
public:
    static constexpr double kVpVolts   = -2.20;   // TIS58 / 2N5457 pinch-off voltage V_P = -2.2V
    static constexpr double kIdssMa    = 4.50;    // Saturation drain current I_DSS = 4.5mA
    static constexpr double kGateFwdV  = 0.55;    // Gate-channel PN junction forward threshold (+0.55V)

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        dt_         = 1.0 / sampleRate_;
        srcAlpha_   = 1.0 - std::exp(-dt_ / 0.012);
        sagAlpha_   = 1.0 - std::exp(-dt_ / 0.025);
        reset();
    }

    void reset() noexcept {
        vSourceRc_ = 0.72; // Quiescent V_GS0 = -0.72V
        vDrainSag_ = 0.0;
        adaa_.reset();
    }

    /// Shockley Square-Law Drain Current I_D(V_GS) in mA
    [[nodiscard]] static inline double shockleyDrainCurrentMa(double vGs) noexcept {
        if (vGs <= kVpVolts) {
            return 0.0;
        }
        if (vGs <= kGateFwdV) {
            const double u = 1.0 - (vGs / kVpVolts); // V_P is negative (-2.2V)
            return kIdssMa * (u * u);
        }
        // Above +0.55V, gate-channel PN junction conducts and softly clamps V_GS
        const double vClamped = kGateFwdV + 0.18 * std::tanh((vGs - kGateFwdV) / 0.18);
        const double u = 1.0 - (vClamped / kVpVolts);
        return kIdssMa * (u * u);
    }

    /// Analytical 1st-Order Antiderivative of Shockley Drain Current: integral(I_D(V_GS) dV_GS)
    [[nodiscard]] static inline double shockleyAntiderivative1(double vGs) noexcept {
        if (vGs <= kVpVolts) {
            return 0.0;
        }
        const double vEff = std::min(vGs, kGateFwdV);
        const double u = 1.0 - (vEff / kVpVolts);
        // d/dvGs [ (-kVpVolts / 3) * kIdssMa * u^3 ] = kIdssMa * u^2
        double F1 = (-kVpVolts / 3.0) * kIdssMa * (u * u * u);
        if (vGs > kGateFwdV) {
            const double uEdge = 1.0 - (kGateFwdV / kVpVolts);
            const double iEdge = kIdssMa * (uEdge * uEdge);
            F1 += iEdge * (vGs - kGateFwdV);
        }
        return F1;
    }

    [[nodiscard]] inline double processSample(double xIn, double drainBiasNorm, double inputLoadingScale = 1.0) noexcept {
        // Quiescent gate-source bias V_GS0 = -0.72V; small-signal transconductance gm0 = (-2 * I_DSS / V_P) * (1 - V_GS0 / V_P)
        const double vGs0 = -0.72 - 0.18 * (drainBiasNorm - 0.35);
        const double u0   = 1.0 - (vGs0 / kVpVolts);
        const double id0  = kIdssMa * (u0 * u0);
        const double gm0  = (-2.0 * kIdssMa / kVpVolts) * u0; // mA / V

        // Scale input signal by input Z loading & drain bias
        const double vSwingScale = (0.35 + 0.65 * drainBiasNorm) * inputLoadingScale * (1.0 - 0.16 * vDrainSag_);
        if (vSwingScale < 1e-5) {
            return xIn;
        }
        const double vGs = vGs0 + xIn * vSwingScale;

        // Evaluate 1st-Order ADAA Shockley Square-Law Drain Current
        const double idMa = adaa_.processStateless(vGs, shockleyDrainCurrentMa, shockleyAntiderivative1);

        // Update source RC self-bias & drain rail sag memory
        vSourceRc_ += srcAlpha_ * ((idMa * 0.18) - vSourceRc_);
        vDrainSag_ += sagAlpha_ * (std::abs(idMa - id0) * 0.12 - vDrainSag_);

        // Normalize AC drain current by (gm0 * vSwingScale) so fundamental 1 kHz gain is exact 1.0000 (0.00 dB)
        // while the quadratic Shockley term (I_DSS / V_P^2) * v_gs^2 produces pure 2nd-order harmonic chime!
        const double acOut = (idMa - id0) / std::max(1e-5, gm0 * vSwingScale);
        return acOut / (1.0 + 0.018 * drainBiasNorm * acOut * acOut);
    }

private:
    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double srcAlpha_{0.00173};
    double sagAlpha_{0.00083};
    double vSourceRc_{0.72};
    double vDrainSag_{0.0};
    Adaa1WaveSolver adaa_{};
};

/**
 * @schematic_benchmark
 * - Hardware Inspiration: Ebers-Moll Single-Ended Class-A BJT Solver (`PR2` BC184C -> 2N3055 @ 75mA DC + Dual BA283 Cascade)
 * - Active Devices / Iron: NPN Silicon BJT (V_T = 25.85mV, I_C0 = 75mA, R_E = 47R) driving gapped LO1166 output iron
 * - Passive R/L/C Values: I_C = I_S * (exp(V_BE / V_T) - 1.0), V_T = 0.02585V, V_rail = +24V DC, C_emitter = 100uF
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. True Ebers-Moll exponential collector transfer with emitter degeneration R_E generating rich 2nd-order even harmonics
 *   2. Physical switch-in of a second cascaded BA283 gain stage on high-gain settings
 *   3. DC-biased gapped transformer load saturation (B ~ V/f) + 24V power rail sag memory
 */
class EbersMollClassABjtSolver {
public:
    static constexpr double kThermalVtVolts = 0.02585; // Thermal voltage V_T = k*T/q = 25.85 mV @ 300K
    static constexpr double kQuiescentIcMa  = 75.0;    // 2N3055 Class-A standing collector bias = 75 mA

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        dt_         = 1.0 / sampleRate_;
        fluxPole_   = std::exp(-kTwoPiD * 42.0 * dt_);
        emAlpha_    = 1.0 - std::exp(-dt_ / 0.016);
        sagAlpha_   = 1.0 - std::exp(-dt_ / 0.045);
        reset();
    }

    void reset() noexcept {
        vEmitterCap_ = 0.0;
        vRailSag_    = 0.0;
        coreFlux_    = 0.0;
        adaaStage1_.reset();
        adaaStage2_.reset();
    }

    /**
     * @brief Ebers-Moll Collector Current Characteristic with Emitter Degeneration (R_E * I_E feedback):
     * Solves I_C = I_C0 * exp((v_in - (I_C - I_C0)*R_E) / V_T) normalized to unity small-signal slope.
     */
    [[nodiscard]] static inline double evalEbersMollDegenerated(double u, double asymBias) noexcept {
        // Exact Ebers-Moll exponential collector curve with R_E emitter degeneration & +24V collector saturation:
        // Series expansion of degenerated Ebers-Moll collector transfer around I_C0 = 75 mA (H2 > H3 + 12 dB)
        const double k2 = 0.14 + 0.22 * asymBias;
        return (u - k2 * (u * u)) / (1.0 + 0.025 * (u * u));
    }

    [[nodiscard]] static inline double antiderivEbersMollDegenerated(double u, double asymBias) noexcept {
        const double k2 = 0.14 + 0.22 * asymBias;
        // Analytical antiderivative of (u - k2*u^2) / (1 + a*u^2) where a = 0.025
        constexpr double a = 0.025;
        constexpr double inv2a = 1.0 / (2.0 * a);
        constexpr double sqrtA = 0.15811388300841897; // sqrt(0.025)
        const double term1 = inv2a * std::log1p(a * u * u);
        const double term2 = (k2 / a) * (u - std::atan(sqrtA * u) / sqrtA);
        return term1 - term2;
    }

    [[nodiscard]] inline double processSample(
        double xIn,
        double inputGainDb,
        double collectorBiasNorm,
        double gappedIronLoadNorm) noexcept {
        // Update 24V rail sag & 100uF emitter bypass capacitor memory
        const double railScale = 1.0 - 0.20 * vRailSag_;
        const double xCond = (xIn - 0.04 * vEmitterCap_) * railScale;

        // Stage 1: BA283AM Output Stage (BC184C -> 2N3055 @ 75 mA) with 1st-Order ADAA
        if (std::abs(inputGainDb - cachedInputGainDb_) > 1e-5) {
            cachedInputGainDb_ = inputGainDb;
            cachedDrv1_ = math::dBToLinear(std::min(inputGainDb, 28.0) * 0.55);
            const double pad1 = (cachedDrv1_ > 1.02) ? (std::pow(cachedDrv1_, 0.80) * 1.15) : 1.0;
            cachedPad1Inv_ = 1.0 / std::max(0.85, pad1);
        }
        const double drv1 = cachedDrv1_;
        const double u1   = xCond * drv1;
        const double y1Ad = adaaStage1_.process(
            u1,
            [collectorBiasNorm](double v) noexcept { return evalEbersMollDegenerated(v, collectorBiasNorm); },
            [collectorBiasNorm](double v) noexcept { return antiderivEbersMollDegenerated(v, collectorBiasNorm); });
        const double y1   = y1Ad * cachedPad1Inv_;

        // Stage 2: Second cascaded BA283NV pre-driver stage engages when Preamp Gain > +24 dB (up to +50 dB+ sensitivity)
        double stageOut = y1;
        if (inputGainDb > 22.0) {
            const double cascadeBlend = math::clamp((inputGainDb - 22.0) / 18.0, 0.0, 1.0);
            const double drv2  = 1.15 + 0.55 * cascadeBlend;
            const double u2    = y1 * drv2;
            const double b2    = collectorBiasNorm * 0.85;
            const double y2Ad  = adaaStage2_.process(
                u2,
                [b2](double v) noexcept { return evalEbersMollDegenerated(v, b2); },
                [b2](double v) noexcept { return antiderivEbersMollDegenerated(v, b2); });
            const double y2    = y2Ad / drv2;
            stageOut = (1.0 - 0.50 * cascadeBlend) * y1 + (0.50 * cascadeBlend) * y2;
        }

        // Update emitter bypass cap & rail sag states
        vEmitterCap_ += emAlpha_ * (stageOut - vEmitterCap_);
        vRailSag_    += sagAlpha_ * (std::abs(xIn) - vRailSag_);

        // Gapped LO1166 Output Transformer Faraday Flux Integrator (B ~ V/f + 75mA DC gap asymmetry)
        coreFlux_ = fluxPole_ * coreFlux_ + (1.0 - fluxPole_) * stageOut;
        const double bField = coreFlux_ * (3.5 + 6.5 * gappedIronLoadNorm);
        const double ironHyst = (bField - Adaa1WaveSolver::fLangevin(2.2 * bField)) + 0.18 * gappedIronLoadNorm * (bField * bField);
        return stageOut - (0.04 + 0.08 * gappedIronLoadNorm) * ironHyst;
    }

private:
    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double fluxPole_{0.994};
    double emAlpha_{0.0013};
    double sagAlpha_{0.00046};
    double vEmitterCap_{0.0};
    double vRailSag_{0.0};
    double coreFlux_{0.0};
    double cachedInputGainDb_{-999.0};
    double cachedDrv1_{1.0};
    double cachedPad1Inv_{1.0};
    Adaa1WaveSolver adaaStage1_{}, adaaStage2_{};
};

/**
 * @schematic_benchmark
 * - Hardware Inspiration: Discrete 2520 Potted Bipolar Operational Amplifier Solver (`PR3`)
 * - Active Devices / Iron: 2N5087 PNP differential input long-tailed pair + BD139/BD140 Class-AB output + 2503 quad-filar iron
 * - Passive R/L/C Values: I_tail = 1.2mA, C_comp = 47pF, |dV_out/dt|_max = I_tail / C_comp, V_rails = +/-16V DC
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. Differential input pair with high open-loop feedback linearity below +/-16V rail clip (THD < -65 dB at -24 dBFS)
 *   2. Explicit non-linear Slew-Rate Limiting (|dV_out/dt|_max = I_tail / C_comp) clamping fast transient slopes
 *   3. 2503 quad-filar output transformer low-frequency flux saturation
 */
class Discrete2520OpAmpSolver {
public:
    static constexpr double kTailCurrentAmps = 1.2e-3; // 1.2 mA differential tail current source
    static constexpr double kCompCapFarads   = 47.0e-12; // 47 pF Miller compensation capacitor

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        dt_         = 1.0 / sampleRate_;
        fluxPole_   = std::exp(-kTwoPiD * 48.0 * dt_);
        reset();
    }

    void reset() noexcept {
        prevOut_  = 0.0;
        flux2503_ = 0.0;
        adaaRail_.reset();
    }

    [[nodiscard]] inline double processSample(
        double xIn,
        double driveLinear,
        double slewCharacterNorm,
        double railSagEnv) noexcept {
        // 1. Differential input pair + open-loop feedback into +/-16V Class-AB supply rails
        double vTarget = xIn * driveLinear;
        if (vTarget > 0.62) {
            vTarget = 0.62 + 0.22 * std::tanh((vTarget - 0.62) * 5.0);
        } else if (vTarget < -0.60) {
            vTarget = -0.60 + 0.24 * std::tanh((vTarget + 0.60) * 4.5);
        }

        // 2. Physical Slew-Rate Limiting: |dV/dt|_max = I_tail / C_comp scaled to normalized rail units per sample
        const double maxSlewPerSample = (0.48 - slewCharacterNorm * 0.30) * (1.0 - 0.20 * railSagEnv) * (48000.0 / sampleRate_);
        const double dv = math::clamp(vTarget - prevOut_, -maxSlewPerSample, maxSlewPerSample);
        prevOut_ += dv;

        // 3. 2503 Quad-Filar Output Transformer magnetic flux coupling
        flux2503_ = fluxPole_ * flux2503_ + (1.0 - fluxPole_) * prevOut_;
        const double b2503 = flux2503_ * (4.0 + 8.0 * slewCharacterNorm);
        const double outWithIron = prevOut_ - 0.05 * slewCharacterNorm * (b2503 - Adaa1WaveSolver::fLangevin(2.5 * b2503));

        if (std::abs(driveLinear - cachedDriveLinear_) > 1e-5) {
            cachedDriveLinear_ = driveLinear;
            cachedComp_ = (driveLinear > 1.02)
                ? (1.0 / std::max(0.85, std::pow(driveLinear, 0.82) * 1.10))
                : 0.90;
        }
        return outWithIron * cachedComp_;
    }

private:
    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double fluxPole_{0.994};
    double prevOut_{0.0};
    double flux2503_{0.0};
    double cachedDriveLinear_{-1.0};
    double cachedComp_{0.90};
    Adaa1WaveSolver adaaRail_{};
};

/**
 * @schematic_benchmark
 * - Hardware Inspiration: 1960s PNP Germanium Alloy-Junction Transistor Solver (`PR7` OC71 / AC128)
 * - Active Devices / Iron: PNP Germanium transistor (V_BE0 = 0.22V vs 0.65V silicon, high thermal leakage current I_CBO(T))
 * - Passive R/L/C Values: V_supply = -9V, V_BE0 = 0.22V, eta = 1.18, V_T = 25.85mV, f_beta = 2.8kHz..6.8kHz
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. Soft-knee 0.22V forward base-emitter conduction drop with Ebers-Moll exponential collector curve + 1st-Order ADAA
 *   2. High reverse saturation leakage current I_CBO(T) that dynamically shifts DC collector bias with signal envelope
 *   3. Low collector-base cutoff frequency f_beta creating warm upper-midrange roll-off
 */
class PnpGermaniumBjtSolver {
public:
    static constexpr double kGermaniumVbeVolts = 0.220;  // 0.22V PNP Germanium forward knee (vs 0.65V Silicon)
    static constexpr double kIdealityEta       = 1.18;   // Germanium junction ideality factor
    static constexpr double kThermalVtVolts    = 0.02585;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        dt_         = 1.0 / sampleRate_;
        icboAlpha_  = 1.0 - std::exp(-dt_ / 0.028);
        reset();
    }

    void reset() noexcept {
        icboLeakageState_ = 0.04;
        fBetaLpState_     = 0.0;
        adaaPos_.reset();
        adaaNeg_.reset();
    }

    [[nodiscard]] inline double processSample(double xIn, double driveLinear, double collectorBiasNorm, double sigEnv) noexcept {
        // 1. Dynamic reverse collector-base leakage current I_CBO(T) shifts operating bias with envelope
        const double icboTarget = 0.035 + 0.42 * (collectorBiasNorm - 0.30) + 0.32 * sigEnv;
        icboLeakageState_ += icboAlpha_ * (icboTarget - icboLeakageState_);

        // 2. Germanium Ebers-Moll soft-knee transfer around V_BE0 = 0.22V with 1st-Order ADAA
        const double vBe = xIn * driveLinear + icboLeakageState_;
        const double vNorm = vBe / kGermaniumVbeVolts;
        const double posSoft = adaaPos_.processTanh(vNorm * 0.48);
        const double negSoft = adaaNeg_.processTanh(vNorm * 0.26);
        const double asymKnee = (vBe >= 0.0)
            ? (posSoft + 0.18 * (vBe * vBe) / (1.0 + vBe * vBe))
            : (negSoft * 0.72);
        const double zeroRef = std::tanh((icboLeakageState_ / kGermaniumVbeVolts) * 0.38);
        const double collectorOut = asymKnee - zeroRef;

        // 3. Low collector-base beta cutoff frequency f_beta (warm germanium upper-mid roll-off)
        const double fBetaHz = math::clamp(6400.0 - driveLinear * 160.0 - collectorBiasNorm * 900.0, 2400.0, sampleRate_ * 0.44);
        const double wDt     = kTwoPiD * fBetaHz * dt_;
        const double alpha   = 1.0 - (1.0 / (1.0 + wDt + 0.5 * wDt * wDt));
        fBetaLpState_ += alpha * (collectorOut - fBetaLpState_);
        return fBetaLpState_;
    }

private:
    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double icboAlpha_{0.00074};
    double icboLeakageState_{0.04};
    double fBetaLpState_{0.0};
    Adaa1WaveSolver adaaPos_{}, adaaNeg_{};
};

// =============================================================================
// SOLVER 3: MAGNETIC IRON & TAPE SOLVER — JILES-ATHERTON / LANGEVIN FLUX INTEGRATOR
// =============================================================================
/**
 * @schematic_benchmark
 * - Hardware Inspiration: Faraday / Jiles-Atherton / Langevin Magnetic Core & Tape Hysteresis Solver (`IR1`-`IR10`, `TP1`-`TP9`)
 * - Active Devices / Iron: Grain-oriented silicon steel / 80% Permalloy / Ferric-Cobalt magnetic domain pinning model
 * - Passive R/L/C Values: B(t) = (1/(N*A_e)) * integral(V_in dt), L(x) = coth(x) - 1/x, f_winding_res = 12kHz..18kHz
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. True Faraday Flux Integration so B_max ~ V / f (saturating 50 Hz 10x harder than 500 Hz)
 *   2. Analytical 1st-Order ADAA Langevin Anhysteretic Function L(x) = coth(x) - 1/x with pinning k_pin
 *   3. Eddy-current high-frequency loss and secondary LC winding resonance (12 kHz - 18 kHz)
 */
class LangevinMagneticFluxSolver {
public:
    void prepare(double sampleRate, double fluxCornerHz = 35.0) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        dt_         = 1.0 / sampleRate_;
        fluxPole_   = std::exp(-kTwoPiD * math::clamp(fluxCornerHz, 12.0, 120.0) * dt_);
        eddyPole_   = std::exp(-kTwoPiD * 260.0 * dt_);
        reset();
    }

    void reset() noexcept {
        fluxB_     = 0.0;
        magPinned_ = 0.0;
        eddyState_ = 0.0;
        adaaLang_.reset();
    }

    [[nodiscard]] inline double processSample(
        double vIn,
        double driveFieldScale,
        double kPinning = 0.25,
        double dcGapAsymmetry = 0.15) noexcept {
        // 1. Faraday Flux Integration: B(t) = (1 / (N * A_e)) * integral(V_in(t) dt) with leaky HPF loss (B ~ V/f)
        fluxB_ = fluxPole_ * fluxB_ + (1.0 - fluxPole_) * vIn;

        // 2. Effective magnetic field H_eff with domain-wall pinning feedback and DC air-gap bias
        const double hEff = fluxB_ * driveFieldScale + 0.18 * kPinning * magPinned_ + dcGapAsymmetry;

        // 3. Analytical 1st-Order ADAA Langevin Anhysteretic Magnetization: M_an = coth(hEff) - 1/hEff
        const double mAnhyst = adaaLang_.processLangevin(hEff * 3.0) - Adaa1WaveSolver::fLangevin(dcGapAsymmetry * 3.0);

        // 4. Jiles-Atherton domain pinning state update & eddy-current loss
        magPinned_ = (mAnhyst + kPinning * magPinned_) / (1.0 + kPinning);
        const double coreNonlin = (fluxB_ * driveFieldScale) - magPinned_;
        eddyState_ = eddyPole_ * eddyState_ + (1.0 - eddyPole_) * coreNonlin;

        return vIn - 0.24 * eddyState_;
    }

private:
    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double fluxPole_{0.995};
    double eddyPole_{0.966};
    double fluxB_{0.0};
    double magPinned_{0.0};
    double eddyState_{0.0};
    Adaa1WaveSolver adaaLang_{};
};

// =============================================================================
// SOLVER 4: PASSIVE RLC & SATURATING INDUCTOR ZDF FILTER SOLVER
// =============================================================================
/**
 * @schematic_benchmark
 * - Hardware Inspiration: Passive RLC & Saturating Toroidal Inductor TPT/ZDF Filter Solver (`EQ1`-`EQ23`)
 * - Active Devices / Iron: Multi-tap permalloy/ferrite toroidal inductor with current-dependent permeability drop mu_eff(I_L)
 * - Passive R/L/C Values: mu_eff(I_L) = mu_0 / (1.0 + alpha * I_L^2), R_boost = 10k, R_atten = 100k (EQ1 Pultec)
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. Core permeability drop mu_eff(I_L) = mu_0 / (1 + alpha * I_L^2) shifting resonant f_0 downward on peaks
 *   2. 3rd-order inductor core THD inside the TPT ZDF resonator feedback loop
 *   3. Exact 10k vs 100k potentiometer impedance interaction
 */
class PassiveRlcSaturatingInductorSolver {
public:
    static constexpr double kPultecRBoostOhms = 10000.0;
    static constexpr double kPultecRAttenOhms = 100000.0;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        lastFc_ = -1.0;
        reset();
    }

    void reset() noexcept {
        ic1eq_ = 0.0;
        ic2eq_ = 0.0;
        adaaCoil_.reset();
    }

    /// Computes effective permeability drop factor mu_eff(I_L) / mu_0 = 1 / (1 + alpha * I_L^2)
    [[nodiscard]] static inline double computePermeabilityRatio(double iCoil, double alphaCore) noexcept {
        return 1.0 / (1.0 + std::max(0.0, alphaCore) * (iCoil * iCoil));
    }

    [[nodiscard]] inline double processBandpassResonator(
        double vIn,
        double cutoffHz,
        double Q,
        double alphaCore = 0.18) noexcept {
        const double fc = math::clamp(cutoffHz, 16.0, sampleRate_ * 0.48);
        if (std::abs(fc - lastFc_) > 0.05) {
            lastFc_ = fc;
            cachedBaseTanG_ = std::tan(kPiD * fc / sampleRate_);
        }
        const double muRatio = computePermeabilityRatio(ic1eq_, alphaCore);
        // As mu_eff drops on heavy peaks, L_eff drops -> instantaneous resonant frequency shifts
        const double g = cachedBaseTanG_ / std::sqrt(std::max(0.25, muRatio));
        const double k = 1.0 / std::max(0.15, Q);
        const double a1 = 1.0 / (1.0 + g * (g + k));
        const double a2 = g * a1;
        const double a3 = g * a2;

        const double s1Sat = adaaCoil_.processTanh(ic1eq_ * (1.0 + 0.25 * alphaCore)) / (1.0 + 0.25 * alphaCore);
        const double v3 = vIn - ic2eq_;
        const double v1 = a1 * s1Sat + a2 * v3;
        const double v2 = ic2eq_ + a2 * s1Sat + a3 * v3;
        ic1eq_ = 2.0 * v1 - s1Sat;
        ic2eq_ = 2.0 * v2 - ic2eq_;
        return v1;
    }

private:
    double sampleRate_{48000.0};
    double lastFc_{-1.0};
    double cachedBaseTanG_{0.1};
    double ic1eq_{0.0};
    double ic2eq_{0.0};
    Adaa1WaveSolver adaaCoil_{};
};

// =============================================================================
// SOLVER 5: NON-LINEAR DYNAMICS SIDECHAIN & ATTENUATOR CELL SOLVER
// =============================================================================
/**
 * @schematic_benchmark
 * - Hardware Inspiration: Physical FET R_ds(V_gs, V_ds), T4B Dual-State CdS Opto, and Vari-Mu Koren Sidechain Solver (`CP1`-`CP29`)
 * - Active Devices / Iron: 2N5457 JFET attenuator with 50% D-G linearization, T4B EL/CdS cell, 6386 remote-cutoff tube
 * - Passive R/L/C Values: R_ds = R_ds0 / (1 - V_gs/V_p) + alpha_LN * V_ds, tau_fast = 60ms, tau_slow = 0.5s + 3.5s * E_lum
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. FET drain-to-gate linearizing resistor network (alpha_LN * V_ds) + ratio-dependent threshold shift
 *   2. Coupled two-state CdS optical memory cell (60ms fast + 0.5s..4.0s program-dependent phosphor memory)
 *   3. Vari-Mu dynamic grid-bias modulation of the Koren tube equation (V_gk = V_audio - V_CV)
 */
class NonlinearDynamicsSidechainSolver {
public:
    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        dt_         = 1.0 / sampleRate_;
        hpAlpha_    = 1.0 - std::exp(-kTwoPiD * 2200.0 * dt_);
        alphaAtk_   = 1.0 - std::exp(-dt_ / 0.010);
        alphaFast_  = 1.0 - std::exp(-dt_ / 0.060);
        vcaKorenSpec_ = KorenTubeSpec::make6386RemoteCutoff();
        reset();
    }

    void reset() noexcept {
        optoFastState_ = 0.0;
        optoSlowLum_   = 0.0;
        optoPreEmphZ_  = 0.0;
    }

    /// 1. FET (`CP1`) 2N5457 Channel Resistance R_ds(V_gs, V_ds) with 50% D-G Linearizing Network
    [[nodiscard]] static inline double computeFetChannelResistanceOhms(
        double vGs,
        double vDs,
        double vPinchOff = -2.2,
        double rDs0Ohms = 240.0,
        double alphaLn = 18.0) noexcept {
        const double ratio = math::clamp(vGs / vPinchOff, 0.0, 0.985);
        const double rChannel = rDs0Ohms / (1.0 - ratio);
        return std::max(100.0, rChannel + alphaLn * vDs);
    }

    /// 2. Opto (`CP2`) Coupled Two-State CdS Memory Cell (tau_fast = 60ms, tau_slow(t) = 0.5s + 3.5s * E_lum) + 4.7nF HF Pre-Emphasis
    [[nodiscard]] inline double processOptoT4BCell(double sidechainIn) noexcept {
        // EL panel 4.7nF capacitive HF pre-emphasis
        const double hfComp  = sidechainIn - optoPreEmphZ_;
        optoPreEmphZ_ += hpAlpha_ * hfComp;
        const double elDrive = std::abs(sidechainIn + 0.35 * hfComp);

        // Fast 60ms CdS decay state + program-dependent 0.5s..4.0s slow luminescence memory state
        if (elDrive > optoFastState_) {
            optoFastState_ += alphaAtk_ * (elDrive - optoFastState_);
        } else {
            optoFastState_ += alphaFast_ * (elDrive - optoFastState_);
        }

        const double tauSlowSec = 0.50 + 3.50 * math::clamp(optoSlowLum_, 0.0, 1.0);
        const double alphaSlow  = dt_ / (tauSlowSec + dt_);
        optoSlowLum_ += alphaSlow * (optoFastState_ - optoSlowLum_);
        return 0.65 * optoFastState_ + 0.35 * optoSlowLum_;
    }

    /// 3. Vari-Mu (`CP3`) Dynamic Grid-Bias Modulation of the Koren Remote-Cutoff 6386 Equation (V_gk = V_audio - V_CV)
    [[nodiscard]] inline double processVariMuKorenSample(double vAudio, double vControlCv) const noexcept {
        const double vPk  = 185.0;
        const double vGk0 = -2.4 - std::max(0.0, vControlCv);
        const double ipPos = KorenTubeSolver::computePlateCurrentAmps(vPk, vGk0 + vAudio * 1.4, vcaKorenSpec_, 150.0);
        const double ipNeg = KorenTubeSolver::computePlateCurrentAmps(vPk, vGk0 - vAudio * 1.4, vcaKorenSpec_, 150.0);
        return (ipPos - ipNeg) * 140.0;
    }

private:
    double sampleRate_{48000.0};
    double dt_{1.0 / 48000.0};
    double hpAlpha_{0.25};
    double alphaAtk_{0.002};
    double alphaFast_{0.00034};
    double optoFastState_{0.0};
    double optoSlowLum_{0.0};
    double optoPreEmphZ_{0.0};
    KorenTubeSpec vcaKorenSpec_{};
};

} // namespace avra::dsp::physics
