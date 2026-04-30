#include "EuropeanPricer.hpp"
#include <cmath>
#include <numeric>
#include <stdexcept>

static constexpr double Z95 = 1.96; // 95% confidence interval z-score

EuropeanPricer::EuropeanPricer(const PathSimulator& sim,
                               const bs::Params& params)
    : sim_(sim), params_(params) {}

double EuropeanPricer::discountedCallPayoff(const std::vector<double>& path) const {
    double ST     = path.back();
    double payoff = std::max(ST - params_.K, 0.0);
    return std::exp(-params_.r * params_.T) * payoff;
}

double EuropeanPricer::discountedPutPayoff(const std::vector<double>& path) const {
    double ST     = path.back();
    double payoff = std::max(params_.K - ST, 0.0);
    return std::exp(-params_.r * params_.T) * payoff;
}

PricingResult EuropeanPricer::price(bool isCall, int nPaths,
                                    Mode mode, int checkpoints) const {
    if (nPaths <= 0) throw std::invalid_argument("nPaths must be > 0");

    double analyticalPrice = isCall ? bs::callPrice(params_)
                                    : bs::putPrice(params_);

    std::vector<double> payoffs;
    payoffs.reserve(nPaths);

    if (mode == Mode::Naive) {
        for (int i = 0; i < nPaths; ++i) {
            auto path = sim_.simulate();
            payoffs.push_back(isCall ? discountedCallPayoff(path)
                                     : discountedPutPayoff(path));
        }

    } else if (mode == Mode::Antithetic) {
        int pairs = nPaths / 2;
        std::vector<double> p, a;
        for (int i = 0; i < pairs; ++i) {
            sim_.simulateAntithetic(p, a);
            double v1 = isCall ? discountedCallPayoff(p) : discountedPutPayoff(p);
            double v2 = isCall ? discountedCallPayoff(a) : discountedPutPayoff(a);
            payoffs.push_back(0.5 * (v1 + v2));
        }

    } else { // ControlVariate
        int pilotN = std::max(100, nPaths / 10);
        std::vector<double> pilotY(pilotN), pilotX(pilotN);

        for (int i = 0; i < pilotN; ++i) {
            auto path  = sim_.simulate();
            pilotY[i]  = isCall ? discountedCallPayoff(path)
                                : discountedPutPayoff(path);
            double ST  = path.back();
            double disc = std::exp(-params_.r * params_.T);
            pilotX[i]  = disc * std::max(ST - params_.K, 0.0);
        }

        double meanY = std::accumulate(pilotY.begin(), pilotY.end(), 0.0) / pilotN;
        double meanX = std::accumulate(pilotX.begin(), pilotX.end(), 0.0) / pilotN;

        double cov = 0.0, varX = 0.0;
        for (int i = 0; i < pilotN; ++i) {
            double dy = pilotY[i] - meanY;
            double dx = pilotX[i] - meanX;
            cov  += dy * dx;
            varX += dx * dx;
        }
        double cStar = (varX > 1e-12) ? (cov / varX) : 0.0;
        double exX   = bs::callPrice(params_);

        for (int i = 0; i < nPaths; ++i) {
            auto path  = sim_.simulate();
            double y   = isCall ? discountedCallPayoff(path)
                                : discountedPutPayoff(path);
            double ST  = path.back();
            double disc = std::exp(-params_.r * params_.T);
            double x   = disc * std::max(ST - params_.K, 0.0);
            payoffs.push_back(y - cStar * (x - exX));
        }
    }

    int N      = static_cast<int>(payoffs.size());
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

    std::vector<double> conv;
    std::vector<int>    sizes;
    int step    = std::max(1, N / checkpoints);
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