// #pragma once

// #include <memory>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <queue>
// #include <functional>
// #include <vector>

// namespace hamza_web
// {

//     class thread_pool
//     {
//     public:
//         thread_pool(size_t num_threads);
//         ~thread_pool();

//         template <typename F>
//         void enqueue(F &&f);

//     private:
//         std::vector<std::thread> workers;
//         std::queue<std::function<void()>> tasks;
//         std::mutex queue_mutex;
//         std::condition_variable condition;
//         bool stop;
//     };

// };