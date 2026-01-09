/********************************************************************
* libavio/include/Queue.hpp
*
* Copyright (c) 2025  Stephen Rhodes
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*********************************************************************/

#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <deque>
#include <mutex>
#include <condition_variable>
#include <exception>

namespace avio {

template <typename T>
class Queue {
public:
    std::deque<T> queue;
    mutable std::mutex mutex;
    std::condition_variable cv_empty;
    std::condition_variable cv_full;
    int64_t max_size;

    explicit Queue(int64_t max_size=-1) : max_size(max_size) {
        // negative max size allows unbounded queue growth 
        if (max_size == 0)
            throw std::runtime_error("Queue size cannot be 0");
    } 

    void push(T&& element) {
        std::unique_lock<std::mutex> lock(mutex);
        cv_full.wait(lock, [&] { return !(queue.size() >= max_size); });
        queue.push_back(std::move(element));
        lock.unlock();
        cv_empty.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        cv_empty.wait(lock, [&] { return !queue.empty(); });
        T result = std::move(queue.front());
        queue.pop_front();
        lock.unlock();
        cv_full.notify_one();
        return result;
    }

    const T* peek() {
        std::lock_guard<std::mutex> lock(mutex);
        return &queue.front();
    }

    const T* at(size_t index) {
        std::lock_guard<std::mutex> lock(mutex);
        return (index < queue.size()) ? &queue[index] : nullptr;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size() >= max_size;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        int n = queue.size();
        queue.clear();
        cv_full.notify_all();
    }
    
    void erase_front(size_t n) {
        std::lock_guard<std::mutex> lock(mutex);
        if (n >= queue.size()) 
            queue.clear();
        else
            queue.erase(queue.begin(), queue.begin() + n);
        cv_full.notify_all();
    }

    // this method removes all elements except for the most current at the back
    void remove_latency() {
        std::lock_guard<std::mutex> lock(mutex);
        size_t n = queue.size();
        if (n > 1) {
            queue.erase(queue.begin(), queue.begin() + n-1);
            cv_full.notify_all();
        }
    }

    size_t find_pts(int64_t pts) {
        std::lock_guard<std::mutex> lock(mutex);
        if constexpr(std::is_same_v<T, Packet>) {
            for (int i = 0; i < queue.size(); i++) {
                if (queue[i].pts() >= pts)
                    return i;
            }
        }
        return SIZE_MAX;
    }

    size_t find_last_key_frame(size_t starting_index) {
        std::lock_guard<std::mutex> lock(mutex);
        if constexpr(std::is_same_v<T, Packet>) {
            for (int i = starting_index; i >= 0; i--) {
                if (i < queue.size()) {
                    if (queue[i].is_key_frame())
                        return i;
                }
            }
        }
        return SIZE_MAX; 
    }

    size_t find_first_key_frame(size_t starting_index) {
        std::lock_guard<std::mutex> lock(mutex);
        if constexpr(std::is_same_v<T, Packet>) {
            for (int i = starting_index; i < queue.size(); i++) {
                if (queue[i].is_key_frame())
                    return i;
            }
        }
        return SIZE_MAX;
    }
};

}

#endif // QUEUE_HPP