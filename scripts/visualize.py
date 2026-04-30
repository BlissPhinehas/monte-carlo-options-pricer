import subprocess
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

# ── Parameters (must match main.cpp) ──────────────────────────────────────
S0    = 100.0
K     = 100.0
T     = 1.0
r     = 0.05
sigma = 0.20
steps = 252
dt    = T / steps

# ── 1. Simulate GBM paths in Python for visualization ─────────────────────
def simulate_path(S0, r, sigma, T, steps, seed=None):
    rng   = np.random.default_rng(seed)
    dt    = T / steps
    Z     = rng.standard_normal(steps)
    log_returns = (r - 0.5 * sigma**2) * dt + sigma * np.sqrt(dt) * Z
    prices = S0 * np.exp(np.cumsum(log_returns))
    return np.concatenate([[S0], prices])

# ── 2. Black-Scholes analytical call price ─────────────────────────────────
from scipy.stats import norm

def bs_call(S, K, T, r, sigma):
    d1 = (np.log(S / K) + (r + 0.5 * sigma**2) * T) / (sigma * np.sqrt(T))
    d2 = d1 - sigma * np.sqrt(T)
    return S * norm.cdf(d1) - K * np.exp(-r * T) * norm.cdf(d2)

bs_price = bs_call(S0, K, T, r, sigma)

# ── 3. Convergence simulation ──────────────────────────────────────────────
def mc_convergence(S0, K, T, r, sigma, steps, max_paths=50000, checkpoints=100):
    rng       = np.random.default_rng(42)
    discount  = np.exp(-r * T)
    prices    = []
    estimates = []
    sizes     = []

    step_size = max_paths // checkpoints
    batch     = max_paths // checkpoints

    all_payoffs = []
    for i in range(checkpoints):
        Z           = rng.standard_normal((batch, steps))
        log_ret     = (r - 0.5 * sigma**2) * (T/steps) + sigma * np.sqrt(T/steps) * Z
        ST          = S0 * np.exp(log_ret.sum(axis=1))
        payoffs     = discount * np.maximum(ST - K, 0.0)
        all_payoffs.extend(payoffs.tolist())
        estimates.append(np.mean(all_payoffs))
        sizes.append(len(all_payoffs))

    return sizes, estimates

# ── 4. Plot ────────────────────────────────────────────────────────────────
fig = plt.figure(figsize=(14, 10))
fig.patch.set_facecolor('#0f0f0f')
gs  = gridspec.GridSpec(2, 2, figure=fig, hspace=0.4, wspace=0.35)

ax1 = fig.add_subplot(gs[0, :])   # price paths — full width
ax2 = fig.add_subplot(gs[1, 0])   # convergence
ax3 = fig.add_subplot(gs[1, 1])   # payoff distribution

for ax in [ax1, ax2, ax3]:
    ax.set_facecolor('#1a1a1a')
    ax.tick_params(colors='#aaaaaa')
    ax.xaxis.label.set_color('#aaaaaa')
    ax.yaxis.label.set_color('#aaaaaa')
    ax.title.set_color('#ffffff')
    for spine in ax.spines.values():
        spine.set_edgecolor('#333333')

t = np.linspace(0, T, steps + 1)

# ── Panel 1: Simulated GBM paths ──────────────────────────────────────────
n_display = 80
for i in range(n_display):
    path = simulate_path(S0, r, sigma, T, steps, seed=i)
    color = '#e05252' if path[-1] < K else '#52a8e0'
    ax1.plot(t, path, alpha=0.25, linewidth=0.6, color=color)

ax1.axhline(K, color='#f0c040', linewidth=1.2, linestyle='--', label=f'Strike K={K}')
ax1.axhline(S0, color='#888888', linewidth=0.8, linestyle=':', label=f'S0={S0}')
ax1.set_title('Simulated GBM Price Paths  (blue = ITM, red = OTM)', fontsize=12)
ax1.set_xlabel('Time (years)')
ax1.set_ylabel('Stock Price ($)')
ax1.legend(facecolor='#2a2a2a', labelcolor='white', fontsize=9)

# ── Panel 2: Convergence of MC estimate ───────────────────────────────────
sizes, estimates = mc_convergence(S0, K, T, r, sigma, steps)
ax2.plot(sizes, estimates, color='#52e0a8', linewidth=1.5, label='MC Estimate')
ax2.axhline(bs_price, color='#f0c040', linewidth=1.2,
            linestyle='--', label=f'BS Price = {bs_price:.4f}')
ax2.set_title('Convergence of MC Price Estimate', fontsize=12)
ax2.set_xlabel('Number of Paths')
ax2.set_ylabel('Estimated Call Price ($)')
ax2.legend(facecolor='#2a2a2a', labelcolor='white', fontsize=9)

# ── Panel 3: Distribution of payoffs ──────────────────────────────────────
rng      = np.random.default_rng(99)
Z        = rng.standard_normal((50000, steps))
log_ret  = (r - 0.5 * sigma**2) * (T/steps) + sigma * np.sqrt(T/steps) * Z
ST       = S0 * np.exp(log_ret.sum(axis=1))
payoffs  = np.exp(-r * T) * np.maximum(ST - K, 0.0)

ax3.hist(payoffs[payoffs > 0], bins=60, color='#7a6ef0',
         edgecolor='none', alpha=0.85)
ax3.axvline(np.mean(payoffs), color='#f0c040', linewidth=1.5,
            linestyle='--', label=f'Mean = {np.mean(payoffs):.4f}')
ax3.set_title('Distribution of Non-Zero Payoffs', fontsize=12)
ax3.set_xlabel('Discounted Payoff ($)')
ax3.set_ylabel('Frequency')
ax3.legend(facecolor='#2a2a2a', labelcolor='white', fontsize=9)

plt.suptitle('Monte Carlo Options Pricing Engine', fontsize=15,
             color='white', fontweight='bold', y=1.01)

plt.savefig('scripts/options_plot.png', dpi=150,
            bbox_inches='tight', facecolor='#0f0f0f')
print("Plot saved to scripts/options_plot.png")
plt.show()