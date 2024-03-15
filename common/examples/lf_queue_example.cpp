#include "lf_queue.hpp"
#include "thread_utils.hpp"

using namespace Common;

void consumer(LFQueue<int>& lfq) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);

    while(lfq.size() > 0) {
        int block;
        if (lfq.pop(block)) {
            std::cout << "consumer read elem: " << block << " lfq-size: " << lfq.size() << std::endl;
        }
        std::this_thread::sleep_for(1.5s);
    }

    std::cout << "consumer exiting." << std::endl;
}

int main() {
    using namespace Common;
    LFQueue<int> lfq(30, std::allocator<int>{});

    std::thread* consumer_thread = createAndStartThread(-1, "LFQ Consumer", consumer, std::ref(lfq));

    for(int i = 0; i < 50; ++i) {
        ASSERT(lfq.push(i), "Producer attempted to push value to full LFQueue");
        std::cout << "main constructed elem: " << i << " lfq-size: " << lfq.size() << std::endl;

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    consumer_thread->join();
    std::cout << "main exiting." << std::endl;
    return 0;
}