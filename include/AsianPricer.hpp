#pragma once
#include "PathSimulator.hpp"
#include "BlackScholes.hpp"
#include "EuropeanPricer.hpp"

// Monte Carlo pricer for Asian (average-price) options.
//
// Asian options pay based on the AVERAGE price over the life of the
// option, not just the terminal price. For example:
//
//   Asian call payoff = max(S_avg - K, 0)
//   Asian put payoff  = max(K - S_avg, 0)
//
// where S_avg = arithmetic mean of prices at each time step.
//
// There is no closed-form Black-Scholes formula for Asian options —
// that's exactly why Monte Carlo is the right tool here.
// We use antithetic variates for variance reduction.

class AsianPricer {
public:
    AsianPricer(const PathSimulator& sim, const bs::Params& params);

    // isCall = true for call, false for put
    // nPaths = number of simulated paths
    PricingResult price(bool isCall, int nPaths,
                        int checkpoints = 20) const;

private:
    const PathSimulator& sim_;
    bs::Params           params_;

    double arithmeticAverage(const std::vector<double>& path) const;
    double discountedCallPayoff(const std::vector<double>& path) const;
    double discountedPutPayoff (const std::vector<double>& path) const;
};