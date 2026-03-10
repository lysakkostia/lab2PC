#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <string>

using namespace std;

void print_header()
{
    cout << left << setw( 18 ) << "Method"
         << right << setw( 15 ) << "Elements"
         << setw( 15 ) << "Time (us)"
         << setw( 15 ) << "Time (ms)"
         << setw( 15 ) << "Speedup" << "\n";
    cout << string( 78, '-' ) << "\n";
}

void print_result( const string& method, size_t n, long long time_us, double speedup = 1.0 ) {
    cout << left << setw( 18 ) << method
         << right << setw( 15 ) << n
         << setw( 15 ) << time_us
         << setw( 15 ) << fixed << setprecision( 2 ) << time_us / 1000.0
         << setw( 15 ) << fixed << setprecision( 2 ) << speedup << "x\n";
}

long long solve_sequential(const vector<int>& data, long long& out_result)
{
    auto start = chrono::high_resolution_clock::now();

    long long result = 0;
    for ( int val : data )
    {
        if ( val % 7 == 0 )
        {
            result ^= val;
        }
    }

    auto end = chrono::high_resolution_clock::now();

    volatile long long anti_optimization_sink = result;
    out_result = result;

    return chrono::duration_cast<chrono::microseconds>( end - start ).count();
}

int main() {
    vector<size_t> test_sizes = { 1'000'000, 10'000'000, 50'000'000, 100'000'000, 500'000'000, 1'000'000'000 };

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib( 1, 1000 );

    print_header();

    for ( size_t size : test_sizes )
        {
        vector<int> data( size );
        for ( size_t i = 0; i < size; ++i )
        {
            data[i] = distrib(gen);
        }

        long long result_xor = 0;
        long long time_us = solve_sequential( data, result_xor );

        print_result("Sequential", size, time_us, 1.0 );

        volatile long long sink = result_xor;
        cout << sink << endl;
    }

    return 0;
}