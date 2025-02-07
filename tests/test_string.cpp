#include "stdromano/string.h"
#include "stdromano/logger.h"

using namespace stdromano;

int main()
{
    set_log_level(LogLevel::Debug);

    String<> s1;
    STDROMANO_ASSERT(s1.empty());
    STDROMANO_ASSERT(s1.size() == 0);
    STDROMANO_ASSERT(s1.capacity() == 7);

    String<> s2 = "test";
    STDROMANO_ASSERT(s2.size() == 4);
    STDROMANO_ASSERT(s2.capacity() == 7);
    STDROMANO_ASSERT(std::strcmp(s2.c_str(), "test") == 0);

    String<> s3 = "this is a long string";
    STDROMANO_ASSERT(s3.size() == 21);
    STDROMANO_ASSERT(s3.capacity() >= 21);
    
    String<> s4 = s2;
    STDROMANO_ASSERT(s4.size() == 4);
    STDROMANO_ASSERT(std::strcmp(s4.c_str(), "test") == 0);

    String<> s5 = s3;
    STDROMANO_ASSERT(s5.size() == 21);
    
    const char* data = "reference";
    auto s6 = String<>::make_ref(data, 9);
    STDROMANO_ASSERT(s6.is_ref());
    STDROMANO_ASSERT(s6.c_str() == data);

    String<> s7("The answer is {}", 42);
    STDROMANO_ASSERT(s7.size() == 16);
    STDROMANO_ASSERT(std::strcmp(s7.c_str(), "The answer is 42") == 0);

    String<> s8 = "hello";
    s8.push_back('!');
    STDROMANO_ASSERT(s8.size() == 6);
    STDROMANO_ASSERT(std::strcmp(s8.c_str(), "hello!") == 0);

    String<> s9 = "world";
    s8.appends(s9);
    STDROMANO_ASSERT(std::strcmp(s8.c_str(), "hello!world") == 0);

    s9.prependc("hello ", 6);
    STDROMANO_ASSERT(std::strcmp(s9.c_str(), "hello world") == 0);

    STDROMANO_ASSERT(fmt::format("{}", s9) == "hello world");

    log_info("All tests passed");

    return 0;
}