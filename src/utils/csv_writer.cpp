#include "qam_simulator/csv_writer.hpp"

#include <iomanip>

CsvWriter::CsvWriter(const std::string& filename) : ofs_(filename) {}

CsvWriter::~CsvWriter() {
    if (ofs_.is_open()) ofs_.close();
}

void CsvWriter::write_header(const std::string& header) {
    ofs_ << header << "\n";
}

void CsvWriter::write_row(double snr, double ber) {
    ofs_ << std::fixed << std::setprecision(12) << snr << "," << ber << "\n";
}