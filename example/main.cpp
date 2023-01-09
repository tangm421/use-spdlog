
#include <future>
#include <chrono>
#include <atomic>
#include <thread>
#include <ctime> // import srand rand time
#include "logger_wrapper.h"
#include "ZkMSFifo.h"

using namespace std;
using namespace example;


class Initializer
{
public:
    Initializer() {
        logger::set_default_logger(logger::make_multisink_logger(
            "global-logger",
            {
                logger::make_colored_console_sink("%L | %X.%f | %5t | %20!s:%-4# | %^%v%$", logger::debug),
                logger::make_daily_file_sink("./example-log.log", "%5!l | %X.%f | %P | %5t | %20!s:%-4# | %v", logger::trace),
            },
            logger::trace));
        logger::flush_on(logger::debug);
        logger::flush_every(std::chrono::seconds(3));
        LOG_DEBUG << "logger init";

        srand(time(nullptr));
    }
    ~Initializer() {
        LOG_DEBUG << "logger uninit";
        logger::shutdown();
    }
    static std::string genSequence() {
        char r1 = rand() % 26 + 65; // uppercase [A-Z]
        char r2 = rand() % 10 + 48; // number [0-9]
        char r3 = rand() % 26 + 97; // lowercase [a-z]
        char r4 = rand() % 26 + 65; // uppercase [A-Z]
        char r5 = rand() % 10 + 48; // number [0-9]
        char r6 = rand() % 26 + 97; // lowercase [a-z]
        return { r1,r2,r3,r4,r5,r6 };
    }
};
static Initializer s_initializer;


class Action
{
public:
    Action() : m_seq_(s_initializer.genSequence()) {
        LOG_DEBUG << "construct action, seq=" << m_seq_;
    }
    ~Action() = default;

    void process() {
        LOG_INFO << "process action... seq=" << m_seq_;
        std:this_thread::sleep_for(std::chrono::seconds(3));
        m_ret_.set_value(m_seq_);
    }

    std::string m_seq_;
    std::promise<std::string> m_ret_;
};

class ActionProcessor
{
public:
    ActionProcessor() {}
    virtual ~ActionProcessor() {
        stop();
    }
    void addAction(Action* act) {
        m_actions_.add(act);
    }
    bool start() {
        if (!m_running_)
        {
            m_worker_ = std::thread(&ActionProcessor::doWork, this);
        }
        return m_running_;
    }
    void stop() {
        if (m_running_)
        {
            m_running_ = false;
            m_worker_.join();
        }
    }
    void setPeaceStop(bool peace = true) {
        m_peace_stop_ = peace;
    }
protected:
    virtual void doWork() {
        LOG_DEBUG << "worker thread started, id=" << std::this_thread::get_id();
        m_running_ = true;
        while (m_running_)
        {
            auto action = m_actions_.getNext(std::chrono::milliseconds(10));
            if (action)
            {
                action->process();
            }
        }
        cleanup();
        m_running_ = false;
        LOG_DEBUG << "worker thread finished, id=" << std::this_thread::get_id();
    }
    virtual void cleanup() {
        if (m_peace_stop_)
        {
            LOG_DEBUG << "worker thread wait for last actions processed";
            while (!m_actions_.empty())
            {
                auto action = m_actions_.getNext();
                action->process();
            }
        }
        else
        {
            while (!m_actions_.empty())
            {
                m_actions_.getNext(); // get the left actions and destroy it automatically
            }
        }
    }

    bool m_peace_stop_ = false;
    std::thread m_worker_;
    std::atomic_bool m_running_{ false };
    ZKMSFifo<Action> m_actions_;
};

class CPUWaster : public ActionProcessor
{
    void doWork() final {
        LOG_DEBUG << "waster thread started, id=" << std::this_thread::get_id();
        m_running_ = true;
        while (m_running_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            for (int i = 0; i < 60; ++i)
            {
                LOG_TRACE << "This is garbage record for consume CPU resources";
            }
        }
        cleanup();
        m_running_ = false;
        LOG_DEBUG << "waster thread finished, id=" << std::this_thread::get_id();
    }
};


int main()
{
    ActionProcessor pro;
    pro.setPeaceStop();
    pro.start();

    CPUWaster waster;
    waster.start();

    for (int i = 0; i < 10; ++i) {
        auto action = new Action();
        auto ret = action->m_ret_.get_future();
        pro.addAction(action);
        auto status = ret.wait_for(std::chrono::seconds(2));
        if (status == std::future_status::ready)
        {
            try
            {
                LOG_INFO << "future get result=" << ret.get();
            }
            catch (std::future_error& ex)
            {
                LOG_ERROR << "future_error for action, seq=" << action->m_seq_ << ", ex:" << ex.what();
            }
        }
        else if (status == std::future_status::timeout)
        {
            LOG_WARNING << "timeout wait for action, seq=" << action->m_seq_;
        }
        else
        {
            LOG_ERROR << "future status is deferred for action, seq=" << action->m_seq_;
        }
    }

    return 0;
}
