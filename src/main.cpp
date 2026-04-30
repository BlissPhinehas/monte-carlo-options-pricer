#include "BlackScholes.hpp"
#include "PathSimulator.hpp"
#include "EuropeanPricer.hpp"
#include "AsianPricer.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>

void printResult(const std::string& label, const PricingResult& r) {
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\n=== " << label << " ===\n";
    std::cout << "  MC Price:       " << r.price << "\n";
    std::cout << "  Std Error:      " << r.stdError << "\n";
    std::cout << "  95% CI:         [" << r.ciLow << ", " << r.ciHigh << "]\n";
    if (r.analyticalPrice > 0.0)
        std::cout << "  BS Analytical:  " << r.analyticalPrice << "\n";
}

int main() {
    // Option parameters
    bs::Params params;
    params.S     = 100.0;  // spot price
    params.K     = 100.0;  // strike price
    params.T     = 1.0;    // 1 year to expiry
    params.r     = 0.05;   // 5% risk-free rate
    params.sigma = 0.20;   // 20% volatility

    int nPaths = 100000;
    int steps  = 252;      // daily steps (trading days in a year)

    PathSimulator sim(params.S, params.r, params.sigma, params.T, steps);

    std::cout << "Monte Carlo Options Pricer\n";
    std::cout << "S=" << params.S << " K=" << params.K
              << " T=" << params.T << " r=" << params.r
              << " sigma=" << params.sigma << "\n";
    std::cout << "Paths: " << nPaths << "  Steps: " << steps << "\n";

    // European call — all three modes
    EuropeanPricer ep(sim, params);

    auto r1 = ep.price(true, nPaths, EuropeanPricer::Mode::Naive);
    printResult("European Call (Naive)", r1);

    auto r2 = ep.price(true, nPaths, EuropeanPricer::Mode::Antithetic);
    printResult("European Call (Antithetic)", r2);

    auto r3 = ep.price(true, nPaths, EuropeanPricer::Mode::ControlVariate);
    printResult("European Call (Control Variate)", r3);

    // European put
    auto r4 = ep.price(false, nPaths, EuropeanPricer::Mode::Antithetic);
    printResult("European Put (Antithetic)", r4);

    // Asian call and put
    AsianPricer ap(sim, params);

    auto r5 = ap.price(true, nPaths);
    printResult("Asian Call (Antithetic)", r5);

    auto r6 = ap.price(false, nPaths);
    printResult("Asian Put (Antithetic)", r6);

    std::cout << "\nDone.\n";
    return 0;
}