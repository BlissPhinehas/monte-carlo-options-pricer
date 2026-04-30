#include "BlackScholes.hpp"
#include <cmath>
#include <stdexcept>

namespace bs {

double normCDF(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

double normPDF(double x) {
    return std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
}

static void computeD1D2(const Params& p, double& d1, double& d2) {
    if (p.sigma <= 0.0 || p.T <= 0.0)
        throw std::invalid_argument("sigma and T must be positive");

    double sqrtT = std::sqrt(p.T);
    d1 = (std::log(p.S / p.K) + (p.r + 0.5 * p.sigma * p.sigma) * p.T)
         / (p.sigma * sqrtT);
    d2 = d1 - p.sigma * sqrtT;
}

double callPrice(const Params& p) {
    double d1, d2;
    computeD1D2(p, d1, d2);
    double discount = std::exp(-p.r * p.T);
    return p.S * normCDF(d1) - p.K * discount * normCDF(d2);
}

double putPrice(const Params& p) {
    // Put-call parity: P = C - S + K*e^{-rT}
    double discount = std::exp(-p.r * p.T);
    return callPrice(p) - p.S + p.K * discount;
}

double callDelta(const Params& p) {
    double d1, d2;
    computeD1D2(p, d1, d2);
    return normCDF(d1);
}

} // namespace bs