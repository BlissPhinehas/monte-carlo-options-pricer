#include "PathSimulator.hpp"
#include <cmath>
#include <random>
#include <stdexcept>

PathSimulator::PathSimulator(double S0, double r, double sigma,
                             double T, int steps, uint64_t seed)
    : S0_(S0), r_(r), sigma_(sigma), T_(T), steps_(steps), seed_(seed)
{
    if (steps <= 0) throw std::invalid_argument("steps must be > 0");
    if (T <= 0)     throw std::invalid_argument("T must be > 0");
    if (sigma <= 0) throw std::invalid_argument("sigma must be > 0");
    if (S0 <= 0)    throw std::invalid_argument("S0 must be > 0");
}

std::vector<double> PathSimulator::simulate() const {
    thread_local uint64_t callCount = 0;
    std::mt19937_64 rng(seed_ + callCount++);
    std::normal_distribution<double> dist(0.0, 1.0);

    double dt        = T_ / steps_;
    double drift     = (r_ - 0.5 * sigma_ * sigma_) * dt;
    double diffusion = sigma_ * std::sqrt(dt);

    std::vector<double> path(steps_ + 1);
    path[0] = S0_;
    for (int i = 1; i <= steps_; ++i) {
        double z = dist(rng);
        path[i]  = path[i-1] * std::exp(drift + diffusion * z);
    }
    return path;
}

void PathSimulator::simulateAntithetic(std::vector<double>& path,
                                        std::vector<double>& anti) const {
    thread_local uint64_t callCount = 0;
    std::mt19937_64 rng(seed_ + callCount++ + 1000000ULL);
    std::normal_distribution<double> dist(0.0, 1.0);

    double dt        = T_ / steps_;
    double drift     = (r_ - 0.5 * sigma_ * sigma_) * dt;
    double diffusion = sigma_ * std::sqrt(dt);

    path.resize(steps_ + 1);
    anti.resize(steps_ + 1);
    path[0] = anti[0] = S0_;

    for (int i = 1; i <= steps_; ++i) {
        double z = dist(rng);
        path[i] = path[i-1] * std::exp(drift + diffusion *  z);
        anti[i] = anti[i-1] * std::exp(drift + diffusion * -z);
    }
}
