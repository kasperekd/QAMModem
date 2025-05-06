# QAM Simulator

## Prerequisites

### 1. Install Required Tools
Install the following packages on a Linux system (Ubuntu/Debian):

```bash
sudo apt-get update
sudo apt-get install g++ cmake python3 python3-pip
```

### 2. Install GoogleTest (Optional)
Install GoogleTest only if you want to run unit tests:

```bash
sudo apt-get install libgtest-dev
```

### 3. Install Python Dependencies
Install required Python libraries for plotting:

```bash
pip3 install pandas matplotlib
```

---

## Build Instructions

### 1. Build Without Tests and SIMD
```bash
cmake -B build -S . -DENABLE_SIMD=OFF -DBUILD_TESTS=OFF -DUSE_PMR=ON
cmake --build build
```

### 2. Build With Tests and SIMD
```bash
cmake -B build -S . -DENABLE_SIMD=ON -DBUILD_TESTS=ON -DUSE_PMR=ON
cmake --build build
ctest --test-dir build
```

### 3. Run the Application
```bash
./build/qam_simulator
```

### 4. Generate BER vs SNR Plot
```bash
cmake --build build --target plot
```
This runs the Python script `plot_ber.py` to generate a plot of BER vs SNR.

---


## Notes

- **SIMD**: Enabled by default (`-DENABLE_SIMD=ON`). Disable with `-DENABLE_SIMD=OFF`.
- **Memory Management**: Uses `std::pmr` by default (`-DUSE_PMR=ON`). Disable with `-DUSE_PMR=OFF`.
- **Customization**: Modify `SimulationParams` in `main.cpp` to adjust SNR ranges, threads, etc.

---

## Results
After running simulations, CSV files are generated. The Python script `plot_ber.py` reads these files and creates a combined plot.

### Example Output:
![BER vs SNR Plot](./img/image.png)  
*Replace `placeholder.png` with your actual screenshot of the BER vs SNR graph.*

---