#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

class ThreadPool
{
public:

  ThreadPool(size_t num_threads = 4): stop(false) 
  {
    threads.reserve(num_threads);
    for(size_t i = 0;i < num_threads;i++) {
      threads.emplace_back([this]
      {
        while(true) {
          std::function<void()> task;

          {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock,[this] {
              return stop || !tasks.empty();
            });

            if (stop && tasks.empty()) {
              return;
            }

            task = std::move(tasks.front());
            tasks.pop();
          }

          task();
        }
      });
    }
  }

  ~ThreadPool()
  {
    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      stop = true;
      cv.notify_all();
    }

    for(auto& thread: threads) {
      thread.join();
    }

  }

  template<typename T>
  void add_task(T&& task)
  {
    {
      std::lock_guard<std::mutex>lock(queue_mutex);
      tasks.emplace(std::move(task));
      cv.notify_one();
    }
  }

private:


  std::queue<std::function<void()>> tasks; 
  std::vector<std::thread> threads;
  std::mutex queue_mutex;
  std::condition_variable cv;
  bool stop = false;
};