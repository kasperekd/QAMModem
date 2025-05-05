#pragma once

#include <cmath>
#include <memory_resource>
#include <random>
#include <span>
#include <utility>
#include <vector>

#ifndef _NUMERIC
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;
#define _NUMERIC
#endif  // !_NUMERIC

/**
 * @brief Класс для добавления AWGN шума к сигналу с поддержкой SIMD
 * оптимизаций.
 *
 * @tparam T Тип данных для представления символов (float/double)
 */
template <Numeric T>
class NoiseAdder {
   public:
    /**
     * @brief Конструктор
     *
     * @param snr_db Уровень SNR в децибелах
     * @param resource Указатель на ресурс памяти
     */
    explicit NoiseAdder(double snr_db, std::pmr::memory_resource* resource =
                                           std::pmr::get_default_resource())
        : snr_db_(snr_db), alloc_(resource) {}

    /**
     * @brief Основной метод добавления шума к сигналу
     *
     * @param symbols Входные символы (реальная и мнимая часть)
     * @return std::pmr::vector<std::pair<T, T>> Символы с добавленным шумом
     */
    std::pmr::vector<std::pair<T, T>> addNoise(
        std::span<const std::pair<T, T>> symbols) const;

   private:
    double snr_db_;
    std::pmr::polymorphic_allocator<T> alloc_;

    // Стандартная реализация (без SIMD)
    std::pmr::vector<std::pair<T, T>> addNoiseStandard(
        std::span<const std::pair<T, T>> symbols) const;

#ifdef ENABLE_SIMD
    // SIMD оптимизированная реализация (AVX2)
    std::pmr::vector<std::pair<T, T>> addNoiseSIMD(
        std::span<const std::pair<T, T>> symbols) const;
#endif
};

#include "src/noise/noise_impl.hpp"