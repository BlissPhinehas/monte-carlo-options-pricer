#pragma once
#include "PathSimulator.hpp"
#include "BlackScholes.hpp"
#include <vector>

// Result bundle returned by every pricer.
struct PricingResult {
    double price;           // MC estimate of option price
    double stdError;        // Standard error of the estimate
    double ciLow;           // 95% confidence interval lower bound
    double ciHigh;          // 95% confidence interval upper bound
    double analyticalPrice; // Black-Scholes analytic price (for comparison)
    std::vector<double> convergence; // Price estimate at each checkpoint
    std::vector<int>    sampleSizes; // Corresponding sample sizes
};

// Monte Carlo pricer for European options.
//
// Three modes:
//   Naive:         simple average of discounted payoffs
//   Antithetic:    average of (path, antithetic path) pairs — reduces variance
//   ControlVariate: uses BS call as a control variate — reduces variance further
//
// Control variate explained simply:
//   We want E[Y] where Y = discounted MC payoff.
//   We also simulate X = discounted BS call payoff on the same path.
//   We KNOW E[X] exactly (it's just the BS formula).
//   New estimator: Y - c*(X - E[X])
//   If Y and X are highly correlated (they are), this kills most of the variance.

class EuropeanPricer {
public:
    enum class Mode { Naive, Antithetic, ControlVariate };

    EuropeanPricer(const PathSimulator& sim, const bs::Params& params);

    // isCall = true for call, false for put
    // nPaths = number of simulated paths
    // mode   = which variance reduction technique to use
    PricingResult price(bool isCall, int nPaths,
                        Mode mode = Mode::Antithetic,
                        int checkpoints = 20) const;

private:
    const PathSimulator& sim_;
    bs::Params           params_;

    double discountedCallPayoff(const std::vector<double>& path) const;
    double discountedPutPayoff (const std::vector<double>& path) const;
};