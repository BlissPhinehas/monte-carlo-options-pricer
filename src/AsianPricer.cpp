#include "AsianPricer.hpp"
#include <cmath>
#include <numeric>
#include <stdexcept>

static constexpr double Z95 = 1.96;

AsianPricer::AsianPricer(const PathSimulator& sim, const bs::Params& params)
    : sim_(sim), params_(params) {}

double AsianPricer::arithmeticAverage(const std::vector<double>& path) const {
    // Skip index 0 (that's S0, the starting price — not part of the average)
    double sum = std::accumulate(path.begin() + 1, path.end(), 0.0);
    return sum / (path.size() - 1);
}

double AsianPricer::discountedCallPayoff(const std::vector<double>& path) const {
    double avg    = arithmeticAverage(path);
    double payoff = std::max(avg - params_.K, 0.0);
    return std::exp(-params_.r * params_.T) * payoff;
}

double AsianPricer::discountedPutPayoff(const std::vector<double>& path) const {
    double avg    = arithmeticAverage(path);
    double payoff = std::max(params_.K - avg, 0.0);
    return std::exp(-params_.r * params_.T) * payoff;
}

PricingResult AsianPricer::price(bool isCall, int nPaths,
                                  int checkpoints) const {
    if (nPaths <= 0) throw std::invalid_argument("nPaths must be > 0");

    std::vector<double> payoffs;
    payoffs.reserve(nPaths);

    // Antithetic variates — same technique as EuropeanPricer
    int pairs = nPaths / 2;
    std::vector<double> p, a;
    for (int i = 0; i < pairs; ++i) {
        sim_.simulateAntithetic(p, a);
        double v1 = isCall ? discountedCallPayoff(p) : discountedPutPayoff(p);
        double v2 = isCall ? discountedCallPayoff(a) : discountedPutPayoff(a);
        payoffs.push_back(0.5 * (v1 + v2));
    }

    int N       = static_cast<int>(payoffs.size());
    double mean = std::accumulate(payoffs.begin(), payoffs.end(), 0.0) / N;

    double var = 0.0;
    for (double v : payoffs) {
        double d = v - mean;
        var += d * d;
    }
    var /= (N - 1);

    double se    = std::sqrt(var / N);
    double ciLow = mean - Z95 * se;
    double ciHi  = mean + Z95 * se;

    // Asian options have no closed-form price so we set analytical to 0.0
    double analyticalPrice = 0.0;

    std::vector<double> conv;
    std::vector<int>    sizes;
    int step       = std::max(1, N / checkpoints);
    double running = 0.0;
    for (int i = 0; i < N; ++i) {
        running += payoffs[i];
        if ((i + 1) % step == 0 || i == N - 1) {
            conv.push_back(running / (i + 1));
            sizes.push_back(i + 1);
        }
    }

    return PricingResult{mean, se, ciLow, ciHi, analyticalPrice, conv, sizes};
}