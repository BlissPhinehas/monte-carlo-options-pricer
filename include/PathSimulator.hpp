#pragma once
#include <vector>
#include <cstdint>

// Generates Geometric Brownian Motion price paths.
//
// The SDE (Stochastic Differential Equation) we're solving is:
//   dS = r*S*dt + sigma*S*dW
//
// Instead of approximating it step by step (Euler method), we use
// the exact log-normal solution:
//   S_{t+dt} = S_t * exp( (r - sigma²/2)*dt + sigma*sqrt(dt)*Z )
//
// where Z ~ N(0,1). This is exact, not an approximation.
//
// Antithetic variates: for every path driven by Z, we also simulate
// a path driven by -Z. The two are negatively correlated, so averaging
// them gives a lower-variance estimate than either path alone.

class PathSimulator {
public:
    // S0    = initial spot price
    // r     = risk-free rate
    // sigma = volatility
    // T     = time to expiry (years)
    // steps = number of time steps per path
    // seed  = random number seed (keeps results reproducible)
    PathSimulator(double S0, double r, double sigma,
                  double T, int steps, uint64_t seed = 42);

    // Simulate one GBM path.
    // Returns a vector of prices at each time step, starting with S0.
    std::vector<double> simulate() const;

    // Simulate a pair of antithetic paths simultaneously.
    void simulateAntithetic(std::vector<double>& path,
                            std::vector<double>& anti) const;

    double getS0()    const { return S0_;    }
    double getT()     const { return T_;     }
    int    getSteps() const { return steps_; }

private:
    double   S0_, r_, sigma_, T_;
    int      steps_;
    uint64_t seed_;
};