#ifndef TIME_MEASURE_H
#define TIME_MEASURE_H

#include <chrono>
#include <iostream>

#define TICK auto _measure_start = std::chrono::high_resolution_clock::now()

#define TOCK(x) auto _measure_end = std::chrono::high_resolution_clock::now();\
    std::chrono::duration<double, std::milli> fp_ms = _measure_end - _measure_start;\
    std::cout << "Elapsed time for: " << x << " " << fp_ms.count() << " ms" << std::endl

#endif // TIME_MEASURE_H
