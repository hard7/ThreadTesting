#include "estimate.h"
#include <iostream>
//#define _GLIBCXX_USE_NANOSLEEP
#include <chrono>
#include <thread>
#include <future>
#include <vector>
#include <algorithm>

#include <iterator>

using std::cout;
using std::endl;

namespace unicorn {
    typedef double value_t;

    template <typename Iterator>
    void accumulate_res(Iterator begin, Iterator end, value_t& res) {
        res = std::accumulate(begin, end, 0);
    }

    template <typename Iterator>
    value_t accumulate(Iterator begin, Iterator end) {
        const std::size_t length = static_cast<std::size_t>(std::distance(begin, end));
        static_cast<double>(length);
        const std::size_t minItemPerThread = 30;
        const std::size_t threadCount = std::min(std::max((std::size_t)std::thread::hardware_concurrency(), 2UL), length / minItemPerThread);

//        cout << "threadCount: " << threadCount << endl;

        const std::size_t piece = length / threadCount;

        std::vector<value_t> result(threadCount, 0);
        std::vector<std::thread> threads(threadCount - 1);

        Iterator cur_begin = begin;
        Iterator cur_end = begin;
        for(int i=0; i<threadCount-1; i++) {
            std::advance(cur_end, piece);
//            cout << " > " << std::distance(begin, cur_begin) << " " << std::distance(begin, cur_end) << endl;
            threads[i] = std::thread(accumulate_res<Iterator>, cur_begin, cur_end, std::ref(result[i]));
            cur_begin = cur_end;
        }
//
        accumulate_res(cur_begin, end, result[threadCount-1]);
        for(std::thread &th : threads) th.join();

//        std::for_each(std::begin(threads), std::end(threads), std::mem_fn(&std::thread::join));
//
        return std::accumulate(std::begin(result), std::end(result), 0);

    }


    template <typename Iterator>
    value_t accumulate_using_future(Iterator begin, Iterator end) {
        const std::size_t length = static_cast<std::size_t>(std::distance(begin, end));
        static_cast<double>(length);
        const std::size_t minItemPerThread = 30;
        const std::size_t threadCount = std::min(std::max((std::size_t)std::thread::hardware_concurrency(), 2UL), length / minItemPerThread);
        const std::size_t piece = length / threadCount;

        std::vector<std::future<value_t> > futures(threadCount-1);
        std::vector<std::thread> threads(threadCount - 1);
//
        Iterator cur_begin = begin;
        Iterator cur_end = begin;
        for(int i=0; i<threadCount-1; i++) {
            std::advance(cur_end, piece);
            std::packaged_task<value_t(Iterator, Iterator, value_t)> task{ std::accumulate<Iterator, value_t> };
            futures[i] = task.get_future();
            threads[i] = std::thread(std::move(task), cur_begin, cur_end, 0);
            cur_begin = cur_end;
        }

        value_t result = std::accumulate(cur_begin, end, 0);
        for(std::thread& thread : threads) thread.join();
        for(auto& future : futures) result += future.get();
        return result;
    }
} // namespace unicon

namespace para {

/**
 * @brief parallel_accumulate
 *
 * @note    based on Listing 2.8,   modifications :
 *          1. using Lambda instead of functor
 *          2. using std::size_t instead of const unigned long
 *          3. using for range instead of std::for_each to join all threads
 *
 * @note    On my laptop (2 cores 4 threads) parallel_accumulate() is faster than
 *          std::accumulate() by around 45%.
 *
 */
    template<typename Iter, typename Value>
    Value parallel_accumulate(Iter first, Iter last, Value init_val)
    {
        using std::size_t;
        size_t length = std::distance(first, last);

        //! trivial case
        if(length == 0) return init_val;

        //! determine number of threads
        size_t min_per_thread = 25;
        size_t max_threads = (length + min_per_thread - 1) / min_per_thread;
        size_t hardware_threads = std::thread::hardware_concurrency();
        size_t num_threads = std::min((hardware_threads!=0 ? hardware_threads : 2), max_threads);
        size_t block_size = length/num_threads;
        std::vector<Value> results(num_threads);
        std::vector<std::thread> threads(num_threads - 1);

        //! construct threads vector
        Iter block_start = first;
        for(size_t idx=0; idx != (num_threads-1);  ++idx )
        {
            Iter block_end = block_start;
            std::advance(block_end, block_size);

            //! @brief  using lambda to construct the threads vector
            //! @attention  block_start and block_end must be captured by reference.
            //!             Otherwise all threads will reference the same objects, which is an
            //!             undefined behaviour.
            threads[idx] = std::thread
                    {
                            [&results, block_start,block_end, idx]{
                                results[idx] = std::accumulate(block_start, block_end, results[idx]);
                            }
                    };

            block_start = block_end;
        }
        results.back() = std::accumulate(block_start, last, results.back());

        //! join all threads and return result
        for(auto& t : threads)  t.join();
        return std::accumulate(results.begin(), results.end(), init_val);
    }
}//namespace para

template<typename Iter, typename Value>
Value serial_accumulate(Iter first, Iter last, Value init_val) {
    for( ; first!=last; ++first) init_val += *first;
    return init_val;
}

template<typename Iter, typename Value>
void accumulate_res(Iter first, Iter last, /*volatile*/ Value& res) {
    res = 0;
    for( ; first!=last; ++first) res += *first;
}

void empty_task() {}

int main() {

    std::vector<unicorn::value_t> array;
    volatile unicorn::value_t res;

    {
        estimate e("Allocate");
        array.resize(1000 * 1000 * 20);
    }

    {
        estimate e("Generate");
        std::generate(std::begin(array), std::end(array), [] { return /*std::rand() % */ 2; });
    }


    int count = 10;

    {
        estimate e("Estimate parallel (pure):");
        for(int i=0;i<count; i++) res = unicorn::accumulate(std::begin(array), std::end(array));
    }

    {
        estimate e("Estimate parallel (lambda):");
        for(int i=0;i<count; i++) res = para::parallel_accumulate(std::begin(array), std::end(array), 0);
    }

    {
        estimate e("Estimate parallel (future):");
        for(int i=0;i<count; i++) res = unicorn::accumulate_using_future(std::begin(array), std::end(array));
    }

    {
        estimate e("Estimate serial (stl):");
        for(int i=0;i<count; i++) res = std::accumulate(std::begin(array), std::end(array), 0);
    }

    {
        estimate e("Estimate serial (mine):");
        for(int i=0;i<count; i++) res = serial_accumulate(std::begin(array), std::end(array), 0);
    }

    return 0;
}