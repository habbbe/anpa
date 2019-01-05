#ifndef TIME_MEASURE_H
#define TIME_MEASURE_H

#include <chrono>
#include <iostream>

#define TICK auto t1 = std::chrono::high_resolution_clock::now();

#define TOCK auto t2 = std::chrono::high_resolution_clock::now();\
    std::chrono::duration<double, std::milli> fp_ms = t2 - t1;\
    std::cout << "Elapsed time: " << fp_ms.count() << " ms" << std::endl;

#endif // TIME_MEASURE_H
