# import pandas as pd
# import matplotlib.pyplot as plt

# df = pd.read_csv('./build/ber_qpsk.csv')
# plt.semilogy(df['SNR_dB'], df['BER'], marker='o')
# plt.xlabel('SNR, dB')
# plt.ylabel('BER')
# plt.grid(True, which='both')
# plt.savefig('ber_vs_snr.png')

import pandas as pd
import matplotlib.pyplot as plt
import glob
import os

file_paths = glob.glob('./ber_*.csv')

plt.figure(figsize=(10, 6))

for file_path in file_paths:
    df = pd.read_csv(file_path)

    filename = os.path.basename(file_path)
    config_name = os.path.splitext(filename)[0][4:]

    plt.semilogy(df['SNR_dB'], df['BER'], marker='o', label=config_name)

plt.xlabel('SNR, dB')
plt.ylabel('BER')
plt.grid(True, which='both')
plt.legend(title='Configuration')
plt.title('BER vs SNR ')

plt.tight_layout()
# plt.savefig('ber_vs_snr_combined.png')

plt.show()