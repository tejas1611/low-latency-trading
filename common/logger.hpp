#pragma once
#include <string>
#include <fstream>
#include <atomic>

#include "lf_queue.hpp"
#include "macros.hpp"
#include "thread_utils.hpp"
#include "time_utils.hpp"

namespace Common {
    constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

    enum class LogType : int8_t {
        CHAR = 0,
        INTEGER = 1,
        LONG_INTEGER = 2,
        LONG_LONG_INTEGER = 3,
        UNSIGNED_INTEGER = 4,
        UNSIGNED_LONG_INTEGER = 5,
        UNSIGNED_LONG_LONG_INTEGER = 6,
        FLOAT = 7,
        DOUBLE = 8
    };

    struct LogElement {
        LogType type_ = LogType::CHAR;
        union {
            char c;
            int i;
            long l;
            long long ll;
            unsigned u;
            unsigned long ul;
            unsigned long long ull;
            float f;
            double d;      
        } data_;
    };

    class Logger final {
    private:
        const std::string file_name_;
        std::ofstream file_;
        LFQueue<LogElement> log_queue_;
        std::thread* logger_thread_ = nullptr;
        std::atomic<bool> running_ = {true};

        LogElement log_element_;

    public:
        explicit Logger(const std::string& file_name) : file_name_(file_name), log_queue_(LOG_QUEUE_SIZE) {
            file_.open(file_name);
            ASSERT(file_.is_open(), "Failed to open log file: " + file_name);
            logger_thread_ = createAndStartThread(-1, "Logger for " + file_name, [this](){ flushQueue(); });
            ASSERT(logger_thread_ != nullptr, "Could not start Logger thread");
        }

        void flushQueue() noexcept {
            while(running_) {
                while (log_queue_.size() && log_queue_.pop(log_element_)) {
                    switch (log_element_.type_) {
                        case LogType::CHAR:
                            file_ << log_element_.data_.c;
                        break;
                        case LogType::INTEGER:
                            file_ << log_element_.data_.i;
                        break;
                        case LogType::LONG_INTEGER:
                            file_ << log_element_.data_.l;
                        break;
                        case LogType::LONG_LONG_INTEGER:
                            file_ << log_element_.data_.ll;
                        break;
                        case LogType::UNSIGNED_INTEGER:
                            file_ << log_element_.data_.u;
                        break;
                        case LogType::UNSIGNED_LONG_INTEGER:
                            file_ << log_element_.data_.ul;
                        break;
                        case LogType::UNSIGNED_LONG_LONG_INTEGER:
                            file_ << log_element_.data_.ull;
                        break;
                        case LogType::FLOAT:
                            file_ << log_element_.data_.f;
                        break;
                        case LogType::DOUBLE:
                            file_ << log_element_.data_.d;
                        break;
                    }
                }

                file_.flush();

                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(10ms);
            }
        }

        ~Logger() {
            std::string time_str;
            std::cerr << Common::getCurrentTimeStr(&time_str) << " Flushing and closing Logger for " << file_name_ << std::endl;

            using namespace std::literals::chrono_literals;
            while (log_queue_.size()) {
                std::this_thread::sleep_for(1s);
            }
            running_ = false;
            logger_thread_->join();

            file_.close();
            std::cerr << Common::getCurrentTimeStr(&time_str) << " Logger for " << file_name_ << " exiting." << std::endl;
        }

        auto pushValue(const LogElement &log_element) noexcept {
            ASSERT(log_queue_.push(log_element), std::string("Logger for ") + file_name_ +  std::string(" attempted to push value to full LFQueue"));
            // *(log_queue_.getNextToWrite()) = log_element;
            // log_queue_.updateWriteIndex();
        }

        auto pushValue(const char value) noexcept {
            pushValue(LogElement{LogType::CHAR, {.c = value}});
        }

        auto pushValue(const int value) noexcept {
            pushValue(LogElement{LogType::INTEGER, {.i = value}});
        }

        auto pushValue(const long value) noexcept {
            pushValue(LogElement{LogType::LONG_INTEGER, {.l = value}});
        }

        auto pushValue(const long long value) noexcept {
            pushValue(LogElement{LogType::LONG_LONG_INTEGER, {.ll = value}});
        }

        auto pushValue(const unsigned value) noexcept {
            pushValue(LogElement{LogType::UNSIGNED_INTEGER, {.u = value}});
        }

        auto pushValue(const unsigned long value) noexcept {
            pushValue(LogElement{LogType::UNSIGNED_LONG_INTEGER, {.ul = value}});
        }

        auto pushValue(const unsigned long long value) noexcept {
            pushValue(LogElement{LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull = value}});
        }

        auto pushValue(const float value) noexcept {
            pushValue(LogElement{LogType::FLOAT, {.f = value}});
        }

        auto pushValue(const double value) noexcept {
            pushValue(LogElement{LogType::DOUBLE, {.d = value}});
        }

        auto pushValue(const char *value) noexcept {
            while (*value) {
                pushValue(*value);
                ++value;
            }
        }

        auto pushValue(const std::string &value) noexcept {
            pushValue(value.c_str());
        }

        template<typename T, typename... A>
        auto log(const char *s, const T &value, A... args) noexcept {
            while (*s) {
                if (*s == '%') {
                    if (*(s + 1) == '%') [[unlikely]] {
                        ++s;
                    } 
                    else {
                        pushValue(value);
                        log(s + 1, args...);
                        return;
                    }
                }
                pushValue(*s++);
            }
            FATAL("extra arguments provided to log()");
        }

        auto log(const char *s) noexcept {
            while (*s) {
                if (*s == '%') {
                    if (*(s + 1) == '%') [[unlikely]] {
                        ++s;
                    } 
                    else {
                        FATAL("missing arguments to log()");
                    }
                }
                pushValue(*s++);
            }
        }

        Logger(const Logger&) = delete;
        Logger(const Logger&&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(const Logger&&) = delete;
    };
}