#pragma once
#include <string>

// Analytic Black-Scholes pricer for European options.
//
// We use this in two ways:
//   1) As a ground-truth reference to verify our MC results.
//   2) As a "control variate" in variance reduction — we know its
//      exact price analytically, so we exploit the correlation between
//      the MC payoff and the BS price to shrink our estimator's variance.

namespace bs {

struct Params {
    double S;     // Current spot price
    double K;     // Strike price
    double T;     // Time to expiry (years)
    double r;     // Risk-free rate (e.g. 0.05 = 5%)
    double sigma; // Volatility (e.g. 0.20 = 20%)
};

double normCDF(double x);   // Standard normal CDF
double normPDF(double x);   // Standard normal PDF

double callPrice(const Params& p);  // BS call price
double putPrice(const Params& p);   // BS put price
double callDelta(const Params& p);  // dC/dS

} // namespace bs