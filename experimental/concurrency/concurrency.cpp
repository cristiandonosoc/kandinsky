#include <kandinsky/core/defines.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

class Timer {
    std::chrono::high_resolution_clock::time_point start;

   public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Elapsed: " << duration.count() << "ms\n";
    }
};

struct ThreadContext {
    std::thread ThreadHandle = {};
    u64 Result = 0;
    u64 Start = 0;
    u64 BlockSize = 0;
};

void Accumulate(ThreadContext* tc) {
    u64 result = 0;
    for (u64 i = tc->Start; i < tc->Start + tc->BlockSize; i++) {
        result += i;
    }

    tc->Result = result;
}

int main() {
    u64 first = 0;
    u64 last = 10000000000;
    {
        Timer timer;
        u64 reference = 0;

        for (u64 i = first; i < last; i++) {
            reference += i;
        }
        std::cout << "REFERENCE: " << reference << std::endl;
    }

    u32 num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 2;
    }

    std::cout << "Threads: " << num_threads << std::endl;

    u64 diff = last - first;
    u64 block_size = diff / num_threads;

    u64 start = first;

    {
        Timer timer;

        // Create all the threads.
        std::vector<ThreadContext> thread_contexts(num_threads - 1);
        for (u32 i = 0; i < num_threads - 1; i++) {
            ThreadContext& thread_context = thread_contexts[i];
            thread_context = {
                .Start = start,
                .BlockSize = block_size,
            };
            thread_context.ThreadHandle = std::thread(Accumulate, &thread_context);
            start += block_size;
        }

        u64 rest = 0;
        for (u64 i = start; i < last; i++) {
            rest += i;
        }

        // Join all the threads.
        for (auto& thread_context : thread_contexts) {
            thread_context.ThreadHandle.join();
            rest += thread_context.Result;
        }

        std::cout << "Multi-threaded result: " << rest << std::endl;
    }
}
