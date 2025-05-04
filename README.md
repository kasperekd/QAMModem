1. Установка Google test
2. Python + зависимости 
3. cmake, g++ ...

1. **Сборка без тестов и без SIMD**:
   ```bash
   cmake -B build -S . -DENABLE_SIMD=OFF -DBUILD_TESTS=OFF -DUSE_PMR=ON
   cmake --build build
   ```

2. **С тестами и SIMD**:
   ```bash
   cmake -B build -S . -DENABLE_SIMD=ON -DBUILD_TESTS=ON -DUSE_PMR=ON
   cmake --build build
   ctest --test-dir build
   ```

3. **Запуск графика**:
   ```bash
   cmake --build build --target plot
   ```

cmake -S .. -DBUILD_APPLICATION=OFF -DBUILD_TESTS=ON -DUSE_PMR=ON
make
