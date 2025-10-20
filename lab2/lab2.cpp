//Автор: Гопко Анастасія група К-27
//Варіант 13
//Компілятор: Microsoft (R) C/C++ Optimizing Compiler
#include <iostream>
#include <vector>
#include <numeric>
#include <chrono>
#include <execution>
#include <thread>
#include <algorithm>
#include <random>
#include <future>
#include <iomanip>

using DurationMs = double;

std::vector<double> generate_data(size_t n) {
    std::vector<double> data(n);
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_real_distribution<float> distrib(0.0, 1.0);

    std::generate(data.begin(), data.end(), [&]() {
        return distrib(generator);
    });

    return data;
}

template <typename Func>
DurationMs measure_time(Func&& func, int runs = 5) {
    DurationMs min_time = std::numeric_limits<DurationMs>::max();

    func();

    for (int i = 0; i < runs; ++i) {
        auto start_time = std::chrono::steady_clock::now();
        func();
        auto end_time = std::chrono::steady_clock::now();
        DurationMs elapsed = std::chrono::duration<DurationMs, std::milli>(end_time - start_time).count();
        min_time = std::min(min_time, elapsed);
    }

    return min_time;
}

template <typename Iterator, typename T>
T custom_parallel_reduce(Iterator first, Iterator last, T init, int K) {
    const size_t distance = std::distance(first, last);
    if (K <= 0 || distance == 0) return init;

    K = std::min(K, (int)distance);
    const size_t part_length = distance / K;
    const size_t remainder = distance % K;

    std::vector<std::future<T>> futures;
    futures.reserve(K);

    Iterator current = first;

    for (int i = 0; i < K; ++i) {
        size_t len = part_length + (i < remainder ? 1 : 0);
        Iterator next = current;
        std::advance(next, len);

        futures.push_back(std::async(std::launch::async,
            [current, next, init]() {
                return std::reduce(std::execution::seq, current, next, init);
            }
        ));

        current = next;
    }

    T result = init;
    for (auto& f : futures) {
        result += f.get();
    }

    return result;
}


void run_experiments(size_t n) {
    auto data = generate_data(n);
    const double sum = 0.0;
    const size_t hardware_concurrency = std::thread::hardware_concurrency();

    std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    std::cout << "Research for data size n = " << n << "\n";
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    std::cout << std::fixed << std::setprecision(3);

    std::cout << "\n--- std::reduce with different policies ---\n";
    std::cout << "| Policy                    | Time (ms) |\n";
    std::cout << "|---------------------------|-----------|\n";

    DurationMs time_default = measure_time([&]() {
        std::reduce(data.begin(), data.end(), sum);
        });
    std::cout << "| No policy                 |  " << std::setw(8) << time_default << " |\n";

    DurationMs time_seq = measure_time([&]() {
        std::reduce(std::execution::seq, data.begin(), data.end(), sum);
        });
    std::cout << "| std::execution::seq       |  " << std::setw(8) << time_seq << " |\n";

    DurationMs time_par = measure_time([&]() {
        std::reduce(std::execution::par, data.begin(), data.end(), sum);
        });
    std::cout << "| std::execution::par       |  " << std::setw(8) << time_par << " |\n";

    DurationMs time_par_unseq = measure_time([&]() {
        std::reduce(std::execution::par_unseq, data.begin(), data.end(), sum);
        });
    std::cout << "| std::execution::par_unseq |  " << std::setw(8) << time_par_unseq << " |\n";


    std::cout << "\n--- Custom Parallel Reduce ---\n";
    std::cout << "(Number of CPU threads = " << hardware_concurrency << ")\n";
    std::cout << "| K (threads) | Time (ms) |\n";
    std::cout << "|-------------|-----------|\n";

    std::vector<int> K_values = { 1, 2, 4 };

    K_values.push_back((int)hardware_concurrency / 2);
    K_values.push_back((int)hardware_concurrency);
    K_values.push_back((int)hardware_concurrency * 2);
    K_values.push_back((int)hardware_concurrency * 4);
    K_values.push_back(64);
    K_values.push_back(100);

    std::sort(K_values.begin(), K_values.end());
    K_values.erase(std::unique(K_values.begin(), K_values.end()), K_values.end());

    DurationMs best_time = std::numeric_limits<DurationMs>::max();
    int best_K = 1;

    for (int K : K_values) {
        if (K > n) continue;

        DurationMs time_custom = measure_time([&]() {
            custom_parallel_reduce(data.begin(), data.end(), sum, K);
            });

        std::cout << "| " << std::setw(10) << K << "  | " << std::setw(8) << time_custom << "  |\n";

        if (time_custom < best_time) {
            best_time = time_custom;
            best_K = K;
        }
        
    }

    std::cout << "Best speed (" << best_time << " ms) achieved with K = " << best_K << "\n";
    std::cout << "Ratio of K_best to CPU threads (" << hardware_concurrency << "): " << (DurationMs)best_K / hardware_concurrency << "\n";
    std::cout << "---------------------------------------------------------\n";
}

int main() {
    std::cout << std::fixed << std::setprecision(3);

    const size_t hardware_concurrency = std::thread::hardware_concurrency();

    std::cout << "Reduce study (adding double).\n";

    const std::vector<size_t> data_sizes = { 1000000, 10000000, 100000000 };

    for (size_t n : data_sizes) {
        run_experiments(n);
    }

    return 0;
}