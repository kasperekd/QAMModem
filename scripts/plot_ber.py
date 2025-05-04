import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('results.csv')
plt.semilogy(df['SNR_dB'], df['BER'], marker='o')
plt.xlabel('SNR, dB')
plt.ylabel('BER')
plt.grid(True, which='both')
plt.savefig('ber_vs_snr.png')