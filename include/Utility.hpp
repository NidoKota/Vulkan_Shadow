#pragma once

#define _USE_MATH_DEFINES

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <string>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <iterator>

#if defined(__ANDROID__)
#include "AndroidOut.h"
#endif

namespace Vulkan_Test
{
    using namespace std;

    //mutex coutMutex;
    inline void coutMultiThread(std::string str)
    {
        //lock_guard<mutex> lock(coutMutex);
#if defined(__ANDROID__)
        aout << str << std::endl;
#else
        cout << str;
#endif
    }
    
    //mutex coutMutex;
    inline void cerrMultiThread(std::string str)
    {
        //lock_guard<mutex> lock(coutMutex);
#if defined(__ANDROID__)
        aout << str << std::endl;
#else
        cerr << str;
#endif
    }

    inline string getTimeStamp()
    {
        auto now = chrono::system_clock::now();
        auto nowC = chrono::system_clock::to_time_t(now);
        auto nowMs = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
        stringstream ss;
        ss << put_time(localtime(&nowC), "%T") << '.' << setfill('0') << setw(3) << nowMs.count();
        return ss.str();
    }

    std::string getIndexStr(size_t index)
    {
        std::stringstream s;
        for (size_t i = 0; i < index; i++)
        {
            s << "    ";
        }
        return s.str();
    }

    template <class T, class UniqueT>
    std::shared_ptr<std::vector<T>> unwrapHandles(std::vector<UniqueT>& uniques)
    {
        std::shared_ptr<std::vector<T>> result = std::make_shared<std::vector<T>>();
        result->reserve(uniques.size());

        std::transform(uniques.begin(), uniques.end(), std::back_inserter(*result),
            [](const UniqueT& unique) -> const T&
            {
                return unique.get();
            }
        );  

        return result;
    }

    int currentLogIndex = 0;

#define SS2STR(x) \
([&]() -> std::string { std::stringstream ss; ss << std::boolalpha << x; return ss.str(); })()

// #define LOG_STR \
// "\n(at <a href=""{" << __FILE__ << "}"" line=""{" << __LINE__ << "}"">" << __FILE__ << ":" << __LINE__ << "</a>)"

#define FILE_LINE_STR \
__FILE__ << ":" << std::setw(3) << std::setfill('0') << __LINE__ << std::setfill(' ')

#define SET_LOG_INDEX(x) \
do { currentLogIndex = x; } while (false)

#define LOG(x) \
do { coutMultiThread(SS2STR(getTimeStamp() << " " << FILE_LINE_STR << " " << getIndexStr(currentLogIndex) << x << "\n")); } while (false)

#define LOGERR(x) \
do { coutMultiThread(SS2STR(getTimeStamp() << " " << FILE_LINE_STR << " " << getIndexStr(currentLogIndex) << x << "\n")); } while (false)
}