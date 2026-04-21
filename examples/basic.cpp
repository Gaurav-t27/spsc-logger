#include "consumer.h"
#include "logger.h"
#include <thread>
#include <vector>

int main() {
    logger::Consumer consumer;

    logger::Logger& log = logger::Logger::instance();

    log.info("starting up");

    std::vector<std::thread> producers;
    for (int i = 0; i < 3; ++i) {
        producers.emplace_back([&log, i] {
            log.debug("thread %d started", i);
            for (int j = 0; j < 5; ++j) {
                log.info("thread %d iteration %d value %f", i, j, j * 1.5);
            }
            log.debug("thread %d done", i);
        });
    }

    for (auto& t : producers)
        t.join();

    log.info("all producers done");

    return 0;
}
