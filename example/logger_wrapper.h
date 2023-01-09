#ifndef __LOGGER_WRAPPER__
#define __LOGGER_WRAPPER__


#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>

#if defined(ENABLE_SPDLOG)
#   include "spdlog/spdlog.h"
#   define CURRENT_FUNCTION SPDLOG_FUNCTION
#else
#   ifndef CURRENT_FUNCTION
#       define CURRENT_FUNCTION static_cast<const char *>(__FUNCTION__)
#   endif
#endif

namespace example
{
    namespace logger
    {
        inline namespace wrapper
        {
#if defined(ENABLE_SPDLOG)
            using spdlog::level::trace;
            using spdlog::level::debug;
            using spdlog::level::info;
            using spdlog::level::warn;
            using spdlog::level::err;
            using spdlog::level::critical;
            using spdlog::level::off;
            using spdlog::level::n_levels;

            using level_enum = spdlog::level::level_enum;
            using level_t = spdlog::level_t;
            using sink_ptr = spdlog::sink_ptr;
            using sinks_init_list = spdlog::sinks_init_list;
            using logger_t = spdlog::logger;
            using logger_ptr = std::shared_ptr<logger_t>;
#else
            enum level_enum {
                trace,
                debug,
                info,
                warn,
                err,
                critical,
                off,
                n_levels
            };
            using level_t = std::atomic<int>;

            class sink
            {
            public:
                virtual ~sink() = default;
                virtual void log(const std::string& msg) = 0;
                virtual void flush() = 0;
                virtual void set_pattern(const std::string& pattern) {}
                void set_level(level_enum log_level);
                level_enum level() const;
                bool should_log(level_enum msg_level) const;
            protected:
                // sink log level - default is all
                level_t level_{ trace };
            };

            class console_sink : public sink
            {
            public:
                console_sink();
                ~console_sink() override;
                console_sink(const console_sink& other) = delete;
                console_sink& operator=(const console_sink& other) = delete;
                void log(const std::string& msg) final override;
                void flush() final override;
            protected:
                static std::mutex& mutex()
                {
                    static std::mutex s_mutex;
                    return s_mutex;
                }
                std::ostream& out_;
                std::mutex& mutex_;
            };
            class file_sink : public sink
            {
            public:
                file_sink(const std::string& filepath, bool truncate);
                ~file_sink() override;
                file_sink(const file_sink& other) = delete;
                file_sink& operator=(const file_sink& other) = delete;
                void log(const std::string& msg) final override;
                void flush() final override;
            private:
                std::string base_filename_;
                bool truncate_;
                std::ofstream out_;
                std::mutex mutex_;
            };
            using sink_ptr = std::shared_ptr<sink>;
            using sinks_init_list = std::initializer_list<sink_ptr>;

            class logger_t
            {
            public:
                // Empty logger
                explicit logger_t(std::string name)
                    : name_(std::move(name))
                {}
                // Logger with range on sinks
                template<typename It>
                logger_t(std::string name, It begin, It end)
                    : name_(std::move(name))
                    , sinks_(begin, end)
                {}
                // Logger with sinks init list
                logger_t(std::string name, sinks_init_list sinks)
                    : logger_t(std::move(name), sinks.begin(), sinks.end())
                {}
                virtual ~logger_t() = default;
                void set_level(level_enum log_level);
                level_enum level() const;
                bool should_log(level_enum msg_level) const;
                void flush_on(level_enum log_level);
                void write_log(level_enum msg_level, const std::string& msg);
            protected:
                bool should_flush_(level_enum msg_level);
                void flush_();
                std::string name_;
                std::vector<sink_ptr> sinks_;
                level_t level_{ logger::info };
                level_t flush_level_{ logger::off };
            };
            using logger_ptr = std::shared_ptr<logger::logger_t>;

            class manager
            {
            public:
                manager(const manager&) = delete;
                manager& operator=(const manager&) = delete;
                static manager& instance();
                void set_default_logger(logger_ptr logger);
                logger_ptr get_default_logger();
            private:
                manager();
                ~manager() = default;
                logger_ptr default_logger_;
            };
#endif
        } // namespace wrapper

        // make a colored console sink
        sink_ptr make_colored_console_sink(const std::string& pattern, level_enum filter_level = info);
        // make a daily file sink which rotates on given time
        sink_ptr make_daily_file_sink(const std::string& filepath, const std::string& pattern, level_enum filter_level = trace,
            uint16_t max_files = 30,
            int rotation_hour = 23,
            int rotation_minute = 59,
            bool truncate = false);
        logger_ptr make_multisink_logger(const std::string& logger_name, sinks_init_list sinks,
            level_enum filter_level = trace);
        void register_logger(logger_ptr logger);
        void set_default_logger(logger_ptr logger);
        logger_ptr get_default_logger();
        void flush_on(level_enum log_level);
        void flush_every(std::chrono::seconds interval);
        void shutdown();
    } // namespace logger

    //////////////////////////////////////////////////////////////////////////
    // Compatible with streaming output
    class LogLine
    {
        std::ostringstream m_ss;
    public:
        LogLine() {}
        template <typename T> LogLine& operator<<(const T& t) { m_ss << t; return *this; }
        std::string str() const { return m_ss.str(); }
    };

    class LogStream
    {
        logger::logger_ptr m_log;
        logger::level_enum m_level_;
        const char* m_file_{ nullptr };
        int m_line_{ 0 };
        const char* m_func_{ nullptr };
    public:
        LogStream(logger::logger_ptr log, logger::level_enum lv, const char* filename_in, int line_in, const char* funcname_in)
            : m_log{ log }
            , m_level_{ lv }
            , m_file_{ filename_in }
            , m_line_{ line_in }
            , m_func_{ funcname_in } {}
        bool operator==(const LogLine& line);
    };
} // namespace example


#if defined(ENABLE_SPDLOG)
//////////////////////////////////////////////////////////////////////////
// Use the fmt mode output of sdplog
#define LOG_WITH_LOGGER_LEVEL_FMT(logger, level, ...)   SPDLOG_LOGGER_CALL((logger), (level), __VA_ARGS__)
#define LOG_WITH_LEVEL_FMT(level, ...)                  LOG_WITH_LOGGER_LEVEL_FMT(example::logger::get_default_logger(), level, __VA_ARGS__)

// !!This macros only available with spdlog, otherwise it does nothing
#define LOG_TRACE_FMT(...)                      LOG_WITH_LEVEL_FMT(example::logger::trace, __VA_ARGS__)
#define LOG_DEBUG_FMT(...)                      LOG_WITH_LEVEL_FMT(example::logger::debug, __VA_ARGS__)
#define LOG_INFO_FMT(...)                       LOG_WITH_LEVEL_FMT(example::logger::info, __VA_ARGS__)
#define LOG_WARNING_FMT(...)                    LOG_WITH_LEVEL_FMT(example::logger::warn, __VA_ARGS__)
#define LOG_ERROR_FMT(...)                      LOG_WITH_LEVEL_FMT(example::logger::err, __VA_ARGS__)
#define LOG_CRITICAL_FMT(...)                   LOG_WITH_LEVEL_FMT(example::logger::critical, __VA_ARGS__)
#else
#define LOG_TRACE_FMT(...)
#define LOG_DEBUG_FMT(...)
#define LOG_INFO_FMT(...)
#define LOG_WARNING_FMT(...)
#define LOG_ERROR_FMT(...)
#define LOG_CRITICAL_FMT(...)
#endif


//////////////////////////////////////////////////////////////////////////
// Compatible with streaming output
#define STREAM_LOG_WITH_LOGGER_LEVEL(logger, level)                                                     \
    logger && logger->should_log(level) &&                                                              \
        example::LogStream(logger, level, __FILE__, __LINE__, CURRENT_FUNCTION) ==                      \
        example::LogLine()
#define LOG_WITH_LOGGER_LEVEL(logger, level)            STREAM_LOG_WITH_LOGGER_LEVEL((logger), (level))
#define LOG_WITH_LEVEL(level)                           LOG_WITH_LOGGER_LEVEL(example::logger::get_default_logger(), level)

#define LOG_TRACE                               LOG_WITH_LEVEL(example::logger::trace)
#define LOG_DEBUG                               LOG_WITH_LEVEL(example::logger::debug)
#define LOG_INFO                                LOG_WITH_LEVEL(example::logger::info)
#define LOG_WARNING                             LOG_WITH_LEVEL(example::logger::warn)
#define LOG_ERROR                               LOG_WITH_LEVEL(example::logger::err)
#define LOG_CRITICAL                            LOG_WITH_LEVEL(example::logger::critical)



#endif // #ifndef __LOGGER_WRAPPER__
