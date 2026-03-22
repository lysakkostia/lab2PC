#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>

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

long long solve_sequential( const vector<int>& data, long long& out_result )
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

long long solve_mutex( const vector<int>& data, int num_threads, long long& out_result )
{
    if ( num_threads > data.size() )
    {
        num_threads = data.size();
    }

    auto start = chrono::high_resolution_clock::now();

    long long global_result = 0;
    mutex mtx;
    vector<thread> threads;

    size_t chunk_size = data.size() / num_threads;
    size_t remainder = data.size() % num_threads;

    auto worker = [&]( size_t start_idx, size_t end_idx )
    {
        for (size_t i = start_idx; i < end_idx; ++i)
        {
            if ( data[i] % 7 == 0 )
            {
                lock_guard<mutex> lock( mtx );
                global_result ^= data[ i ];
            }
        }
    };

    size_t current_start = 0;
    for ( int i = 0; i < num_threads; ++i )
    {
        size_t current_end = current_start + chunk_size + ( i < remainder ? 1 : 0 );
        threads.emplace_back(worker, current_start, current_end);
        current_start = current_end;
    }

    for ( auto& t : threads )
    {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now();
    out_result = global_result;
    return chrono::duration_cast<chrono::microseconds>( end - start ).count();
}

long long solve_atomic_cas( const vector<int>& data, int num_threads, long long& out_result ) {
    if ( num_threads > data.size() )
    {
        num_threads = data.size();
    }

    auto start = chrono::high_resolution_clock::now();


    atomic<long long> global_result(0);
    vector<thread> threads;

    size_t chunk_size = data.size() / num_threads;
    size_t remainder = data.size() % num_threads;

    auto worker = [&]( size_t start_idx, size_t end_idx )
    {
        for ( size_t i = start_idx; i < end_idx; ++i )
        {
            if ( data[i] % 7 == 0 )
            {
                long long current = global_result.load();
                long long desired;
                do {
                    desired = current ^ data[ i ];
                }
                while ( !global_result.compare_exchange_weak(current, desired) );
            }
        }
    };

    size_t current_start = 0;
    for ( int i = 0; i < num_threads; ++i )
    {
        size_t current_end = current_start + chunk_size + ( i < remainder ? 1 : 0 );
        threads.emplace_back( worker, current_start, current_end );
        current_start = current_end;
    }

    for ( auto& t : threads )
    {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now();
    out_result = global_result.load();
    return chrono::duration_cast<chrono::microseconds>( end - start ).count();
}

int main() {

    vector<int> thread_counts = { 2, 4, 8, 16, 32, 64, 128, 256, 512 };
    vector<size_t> test_sizes = { 1'000'000, 10'000'000, 50'000'000, 100'000'000, 1'000'000'000 };

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(1, 1000);

    print_header();

    for ( size_t size : test_sizes )
    {

        vector<int> data( size );
        for ( size_t i = 0; i < size; ++i )
        {
            data[ i ] = distrib( gen );
        }


        long long res_seq = 0;
        long long time_seq = solve_sequential( data, res_seq );
        print_result("Sequential", size, time_seq, 1.0 );


        for ( int num_threads : thread_counts )
        {
            long long res_mutex = 0;
            long long time_mutex = solve_mutex( data, num_threads, res_mutex );

            double speedup = static_cast<double>( time_seq ) / time_mutex;

            string method_name = "Mutex (" + to_string( num_threads ) + "t)";
            print_result( method_name, size, time_mutex, speedup );

            if ( res_seq != res_mutex )
            {
                cout << "ERROR: Results are not the same! Seq: " << res_seq << " Mutex: " << res_mutex << "\n";
            }
            volatile long long sink_mutex = res_mutex;

            long long res_cas = 0;
            long long time_cas = solve_atomic_cas( data, num_threads, res_cas );
            double speedup_cas = static_cast<double>( time_seq ) / time_cas;
            print_result("CAS (" + to_string( num_threads ) + "t)", size, time_cas, speedup_cas );

            if ( res_seq != res_cas )
            {
                cout << "ERROR (CAS): Seq: " << res_seq << " CAS: " << res_cas << "\n";
            }

            volatile long long sink_cas = res_cas;
        }

        cout << string( 78, '-' ) << "\n";
        volatile long long sink_seq = res_seq;
    }

    return 0;
}