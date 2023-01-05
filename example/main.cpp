
#include <future>
#include <chrono>
#include "logger_wrapper.h"

using namespace std;
using namespace example;

using int_promise = std::promise<int>;
using int_future = std::future<int>;
class promise_wrapper : public int_promise
{
public:
    promise_wrapper() {}
    ~promise_wrapper() {
        LOG_DEBUG << "promise are going to be destruct";
    }
    promise_wrapper& operator=(const promise_wrapper& rhs) = delete;
    promise_wrapper(promise_wrapper&& other) noexcept : int_promise(std::move(other)) {}
};

void thread_test(promise_wrapper* pro)
{
    LOG_TRACE << "started thread: " << std::this_thread::get_id();
    std::this_thread::sleep_for(chrono::seconds(5));
    pro->set_value(0);
    delete pro;
}


int main()
{
    logger::set_default_logger(logger::make_multisink_logger(
        "global-logger",
        {
            logger::make_colored_console_sink("%L | %X.%f | %5t | %20!s:%-4# | %^%v%$", logger::trace),
            logger::make_daily_file_sink("./log-example.log", "%5!l | %X.%f | %P | %5t | %20!s:%-4# | %v", logger::trace),
        },
        logger::trace));
    logger::flush_on(logger::debug);
    logger::flush_every(std::chrono::seconds(3));
    LOG_DEBUG << "logger init done";

    promise_wrapper* pro = new promise_wrapper;
    std::thread work_thread(thread_test, pro);

    {
        int_future fut = pro->get_future();
        std::future_status status = fut.wait_for(std::chrono::seconds(3));
        if (status == std::future_status::ready)
        {
            try
            {
                LOG_INFO << "future get: " << fut.get();
            }
            catch (std::future_error& ex)
            {
                LOG_ERROR << "future_error of GetMediaStatistic, ex:" << ex.what();
            }
        }
        else if (status == std::future_status::timeout)
        {
            LOG_WARNING << "timeout wait for GetMediaStatistic";
        }
        else
        {
            LOG_ERROR << "future status: deferred";
        }
    }

    work_thread.join();

    return 0;
}