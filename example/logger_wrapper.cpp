#include  "logger_wrapper.h"

#if defined(ENABLE_SPDLOG)
#   include "spdlog/sinks/stdout_color_sinks.h"
#   include "spdlog/sinks/daily_file_sink.h"
#   include "spdlog/sinks/basic_file_sink.h"
#else
#   include <thread>
#   include <iomanip>
#endif

namespace example
{
    namespace logger
    {
        namespace wrapper
        {
#if !defined(ENABLE_SPDLOG)
            void sink::set_level(level_enum log_level)
            {
                level_.store(log_level, std::memory_order_relaxed);
            }
            level_enum sink::level() const
            {
                return static_cast<level_enum>(level_.load(std::memory_order_relaxed));
            }
            bool sink::should_log(level_enum msg_level) const
            {
                return msg_level >= level_.load(std::memory_order_relaxed);
            }

            console_sink::console_sink() : out_(std::cout), mutex_(mutex())
            {
            }
            console_sink::~console_sink()
            {
                this->flush();
            }
            void console_sink::log(const std::string& msg)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                out_ << msg;
            }
            void console_sink::flush()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                out_.flush();
            }

            file_sink::file_sink(const std::string& filepath, bool truncate)
                : base_filename_()
                , truncate_(truncate)
            {
                out_.open(filepath, std::ios_base::out | std::ios_base::binary | (truncate_ ? std::ios_base::trunc : std::ios_base::app));
            }

            file_sink::~file_sink()
            {
            }
            void file_sink::log(const std::string& msg)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                out_ << msg;
            }
            void file_sink::flush()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                out_.flush();
            }

            void logger_t::set_level(level_enum log_level)
            {
                level_.store(log_level);
            }
            level_enum logger_t::level() const
            {
                return static_cast<level_enum>(level_.load(std::memory_order_relaxed));
            }
            bool logger_t::should_log(level_enum msg_level) const
            {
                return msg_level >= level_.load(std::memory_order_relaxed);
            }

            void logger_t::flush_on(level_enum log_level)
            {
                flush_level_.store(log_level);
            }

            void logger_t::write_log(level_enum msg_level, const std::string& msg)
            {
                for (auto& sink : sinks_)
                {
                    if (sink->should_log(msg_level))
                    {
                        try
                        {
                            sink->log(msg);
                        }
                        catch (const std::exception& ex)
                        {
                            std::cerr << ex.what() << std::endl;
                        }
                        catch (...)
                        {
                            std::cerr << "Rethrowing unknown exception in logger::write_log" << std::endl;
                            throw;
                        }
                    }
                }

                if (should_flush_(msg_level))
                {
                    flush_();
                }
            }

            bool logger_t::should_flush_(level_enum msg_level)
            {
                auto flush_level = flush_level_.load(std::memory_order_relaxed);
                return (msg_level >= flush_level) && (msg_level != logger::off);
            }

            void logger_t::flush_()
            {
                for (auto& sink : sinks_)
                {
                    try
                    {
                        sink->flush();
                    }
                    catch (const std::exception& ex)
                    {
                        std::cerr << ex.what() << std::endl;
                    }
                    catch (...)
                    {
                        std::cerr << "Rethrowing unknown exception in logger::flush_" << std::endl;
                        throw;
                    }
                }
            }

            manager& manager::instance()
            {
                static manager s_instance;
                return s_instance;
            }

            manager::manager()
            {
                auto sink = make_colored_console_sink("", logger::trace);
                sinks_init_list sinks = {sink};
                default_logger_ = std::make_shared<logger_t>("", sinks);
            }

            void manager::set_default_logger(logger_ptr logger)
            {
                // remove previous default logger from the map
                if (default_logger_ != nullptr)
                {
                    default_logger_.reset();
                }
                default_logger_ = std::move(logger);
            }
            logger_ptr manager::get_default_logger()
            {
                return default_logger_;
            }
#endif
        } // namespace wrapper

        sink_ptr make_colored_console_sink(const std::string& pattern, level_enum filter_level /*= info*/)
        {
#if defined(ENABLE_SPDLOG)
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#else
            auto console_sink = std::make_shared<logger::console_sink>();
#endif
            console_sink->set_level(filter_level);
            //console_sink->set_pattern("%L | %X.%f | %t | %20!s:%-4# | %^%v%$");
            console_sink->set_pattern(pattern);
            return console_sink;
        }

        sink_ptr make_daily_file_sink(const std::string& filepath, const std::string& pattern, level_enum filter_level, uint16_t max_files, int rotation_hour, int rotation_minute, bool truncate)
        {
#if defined(ENABLE_SPDLOG)
            auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(filepath, rotation_hour, rotation_minute, truncate, max_files);
#else
            auto file_sink = std::make_shared<logger::file_sink>(filepath, truncate);
#endif
            file_sink->set_level(filter_level);
            file_sink->set_pattern(pattern);
            //file_sink->set_pattern("%5!l | %X.%f | %P | %t | %20!s:%-4# | %v");
            return file_sink;
        }

        logger_ptr make_multisink_logger(const std::string& logger_name, sinks_init_list sinks, level_enum filter_level)
        {
            auto logger = std::make_shared<logger_t>(logger_name, sinks);
            logger->set_level(filter_level);
            logger::register_logger(logger);
            return logger;
        }

        void register_logger(logger_ptr logger)
        {
#if defined(ENABLE_SPDLOG)
            spdlog::register_logger(logger);
#endif
        }

        void set_default_logger(logger_ptr logger)
        {
#if defined(ENABLE_SPDLOG)
            spdlog::set_default_logger(logger);
#else
            manager::instance().set_default_logger(logger);
#endif
        }

        logger_ptr get_default_logger()
        {
#if defined(ENABLE_SPDLOG)
            return spdlog::default_logger();
#else
            return logger::manager::instance().get_default_logger();
#endif
        }

        void flush_on(level_enum log_level)
        {
#if defined(ENABLE_SPDLOG)
            spdlog::flush_on(log_level);
#else
            logger::manager::instance().get_default_logger()->flush_on(log_level);
#endif
        }

        void flush_every(std::chrono::seconds interval)
        {
#if defined(ENABLE_SPDLOG)
            spdlog::flush_every(interval);
#endif
        }

    } // namespace logger


    bool LogStream::operator==(const LogLine& line)
    {
#if defined(ENABLE_SPDLOG)
        m_log->log(spdlog::source_loc{ m_file_,m_line_,m_func_ }, m_level_, "{}", line.str());
#else
        static const char* level_str[] = { "T", "D", "I", "W", "E", "C", "O" };
        std::ostringstream ss;
        const auto now = std::chrono::system_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
        const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        auto file = std::string(m_file_);
        ss << level_str[m_level_]
            << " | " << std::put_time(std::localtime(&now_tt), "%T") << "." << std::setfill('0') << std::setw(6) << us << std::setfill(' ')
            << " | " << std::setw(5) << std::this_thread::get_id()
            << " | " << std::setw(20) << (file.size() < 20 ? file : file.substr(file.size() - 20)) << ":" << std::left << std::setw(4) << m_line_;
        //if (m_func_)
        //{
        //    ss << " | " << m_func_;
        //}
        ss << " | " << line.str() << std::endl;
        m_log->write_log(m_level_, ss.str());
#endif
        return true;
    }

} // namespace example

