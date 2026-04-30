#include <gtest/gtest.h>
#include "BlackScholes.hpp"
#include "PathSimulator.hpp"
#include "EuropeanPricer.hpp"
#include "AsianPricer.hpp"
#include <cmath>

// ── Black-Scholes Tests ────────────────────────────────────────────────────

TEST(BlackScholesTest, CallPriceKnownValue) {
    // Standard textbook values — BS call should be ~10.4506
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    double price = bs::callPrice(p);
    EXPECT_NEAR(price, 10.4506, 0.001);
}

TEST(BlackScholesTest, PutCallParity) {
    // Put-call parity: C - P = S - K*e^{-rT}
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    double C    = bs::callPrice(p);
    double P    = bs::putPrice(p);
    double parity = p.S - p.K * std::exp(-p.r * p.T);
    EXPECT_NEAR(C - P, parity, 0.0001);
}

TEST(BlackScholesTest, CallPriceDeepInTheMoney) {
    // Deep in the money call should be close to S - K*e^{-rT}
    bs::Params p{200.0, 100.0, 1.0, 0.05, 0.20};
    double price = bs::callPrice(p);
    EXPECT_GT(price, 95.0);
}

TEST(BlackScholesTest, CallPriceDeepOutOfTheMoney) {
    // Deep out of the money call should be close to zero
    bs::Params p{50.0, 200.0, 1.0, 0.05, 0.20};
    double price = bs::callPrice(p);
    EXPECT_NEAR(price, 0.0, 0.01);
}

TEST(BlackScholesTest, InvalidSigmaThrows) {
    bs::Params p{100.0, 100.0, 1.0, 0.05, -0.20};
    EXPECT_THROW(bs::callPrice(p), std::invalid_argument);
}

// ── PathSimulator Tests ────────────────────────────────────────────────────

TEST(PathSimulatorTest, PathLengthCorrect) {
    PathSimulator sim(100.0, 0.05, 0.20, 1.0, 252);
    auto path = sim.simulate();
    EXPECT_EQ(static_cast<int>(path.size()), 253); // steps + 1
}

TEST(PathSimulatorTest, PathStartsAtS0) {
    PathSimulator sim(100.0, 0.05, 0.20, 1.0, 252);
    auto path = sim.simulate();
    EXPECT_DOUBLE_EQ(path[0], 100.0);
}

TEST(PathSimulatorTest, PathAlwaysPositive) {
    PathSimulator sim(100.0, 0.05, 0.20, 1.0, 252);
    for (int i = 0; i < 50; ++i) {
        auto path = sim.simulate();
        for (double price : path)
            EXPECT_GT(price, 0.0);
    }
}

TEST(PathSimulatorTest, AntitheticPathsSameLength) {
    PathSimulator sim(100.0, 0.05, 0.20, 1.0, 252);
    std::vector<double> p, a;
    sim.simulateAntithetic(p, a);
    EXPECT_EQ(p.size(), a.size());
}

TEST(PathSimulatorTest, InvalidStepsThrows) {
    EXPECT_THROW(PathSimulator(100.0, 0.05, 0.20, 1.0, 0), std::invalid_argument);
}

// ── EuropeanPricer Tests ───────────────────────────────────────────────────

TEST(EuropeanPricerTest, NaiveCallCloseToBS) {
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    PathSimulator sim(p.S, p.r, p.sigma, p.T, 252);
    EuropeanPricer ep(sim, p);
    auto result = ep.price(true, 100000, EuropeanPricer::Mode::Naive);
    // Within 2% of analytical
    EXPECT_NEAR(result.price, result.analyticalPrice, 0.25);
}

TEST(EuropeanPricerTest, AntitheticTighterThanNaive) {
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    PathSimulator sim(p.S, p.r, p.sigma, p.T, 252);
    EuropeanPricer ep(sim, p);
    auto naive = ep.price(true, 10000, EuropeanPricer::Mode::Naive);
    auto anti  = ep.price(true, 10000, EuropeanPricer::Mode::Antithetic);
    // Antithetic should have lower standard error
    EXPECT_LT(anti.stdError, naive.stdError);
}

TEST(EuropeanPricerTest, ControlVariateVeryAccurate) {
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    PathSimulator sim(p.S, p.r, p.sigma, p.T, 252);
    EuropeanPricer ep(sim, p);
    auto result = ep.price(true, 10000, EuropeanPricer::Mode::ControlVariate);
    EXPECT_NEAR(result.price, result.analyticalPrice, 0.05);
}

TEST(EuropeanPricerTest, PutPriceReasonable) {
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    PathSimulator sim(p.S, p.r, p.sigma, p.T, 252);
    EuropeanPricer ep(sim, p);
    auto result = ep.price(false, 50000, EuropeanPricer::Mode::Antithetic);
    EXPECT_NEAR(result.price, result.analyticalPrice, 0.20);
}

// ── AsianPricer Tests ──────────────────────────────────────────────────────

TEST(AsianPricerTest, AsianCallCheaperThanEuropean) {
    // Asian options are always cheaper than European because averaging
    // the price path reduces the effective volatility.
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    PathSimulator sim(p.S, p.r, p.sigma, p.T, 252);

    EuropeanPricer ep(sim, p);
    AsianPricer    ap(sim, p);

    auto european = ep.price(true, 50000, EuropeanPricer::Mode::Antithetic);
    auto asian    = ap.price(true, 50000);

    EXPECT_LT(asian.price, european.price);
}

TEST(AsianPricerTest, AsianPricePositive) {
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    PathSimulator sim(p.S, p.r, p.sigma, p.T, 252);
    AsianPricer ap(sim, p);
    auto result = ap.price(true, 10000);
    EXPECT_GT(result.price, 0.0);
}

TEST(AsianPricerTest, ConvergenceVectorNotEmpty) {
    bs::Params p{100.0, 100.0, 1.0, 0.05, 0.20};
    PathSimulator sim(p.S, p.r, p.sigma, p.T, 252);
    AsianPricer ap(sim, p);
    auto result = ap.price(true, 10000);
    EXPECT_FALSE(result.convergence.empty());
}