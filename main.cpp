#include "qam_simulator/pipeline.hpp"

int main(int argc, char** argv) {
    SimulationParams params = parse_args(argc, argv);
    run_all_simulations(params);
    return 0;
}
