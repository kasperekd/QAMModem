#pragma once

#include <chrono>
#include <cstdlib>
#include <iostream>

/**
 * @brief Structure containing simulation parameters.
 *
 * This structure holds all the configuration values needed for running a
 * simulation, including SNR range, number of threads, bits per thread,
 * and iterations per SNR value.
 */
struct SimulationParams {
    /**
     * @brief Starting value of the Signal-to-Noise Ratio (SNR) in dB.
     */
    double snr_start;

    /**
     * @brief Ending value of the Signal-to-Noise Ratio (SNR) in dB.
     */
    double snr_end;

    /**
     * @brief Step size between consecutive SNR values.
     */
    double snr_step;

    /**
     * @brief Number of threads to use for parallel execution.
     */
    int num_threads;

    /**
     * @brief Number of bits processed by each thread.
     */
    size_t bits_per_thread;

    /**
     * @brief Number of iterations performed for each SNR value.
     */
    size_t iterations_per_snr;
};

/**
 * @brief Parses command-line arguments and initializes simulation parameters.
 *
 * This function reads the command-line arguments and populates the
 * `SimulationParams` structure with the provided values.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return A `SimulationParams` object initialized from the input arguments.
 */
SimulationParams parse_args(int argc, char** argv);

/**
 * @brief Runs the entire simulation based on the provided parameters.
 *
 * This function orchestrates the execution of the simulation using the
 * given configuration.
 *
 * @param params Simulation parameters to be used for execution.
 */
void run_all_simulations(const SimulationParams&);