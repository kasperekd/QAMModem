#pragma once

#include <fstream>
#include <string>

/**
 * @brief Class for writing data to a CSV file.
 *
 * This class provides functionality to write headers and rows of data
 * in CSV format. It automatically opens a file on construction and
 * closes it upon destruction.
 */
class CsvWriter {
   public:
    /**
     * @brief Constructor that opens the CSV file.
     *
     * @param filename The name of the CSV file to create or overwrite.
     */
    explicit CsvWriter(const std::string& filename);

    /**
     * @brief Destructor that closes the CSV file if it is open.
     */
    ~CsvWriter();

    /**
     * @brief Writes a header line to the CSV file.
     *
     * @param header A string containing the header row (e.g., column names).
     */
    void write_header(const std::string& header);

    /**
     * @brief Writes a row of data to the CSV file.
     *
     * @param snr Signal-to-Noise Ratio value to write.
     * @param ber Bit Error Rate value to write.
     */
    void write_row(double snr, double ber);

   private:
    std::ofstream ofs_;  ///< Output file stream for writing CSV data.
};