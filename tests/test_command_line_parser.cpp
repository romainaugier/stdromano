// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/command_line_parser.hpp"

#include "fixtures.hpp"

#include <string>
#include <vector>

using namespace stdromano;

class Argv
{
    std::vector<std::string> _storage;
    std::vector<char*> _pointers;

public:
    Argv(std::initializer_list<std::string> args) : _storage(args)
    {
        this->_storage.insert(this->_storage.begin(), "program");
    }

    explicit Argv(std::vector<std::string> args) : _storage(std::move(args))
    {
        this->_storage.insert(this->_storage.begin(), "program");
    }

    int argc()
    {
        return static_cast<int>(this->_storage.size());
    }

    char** argv()
    {
        this->_pointers.clear();

        for(std::string& arg : this->_storage)
            this->_pointers.push_back(arg.data());

        this->_pointers.push_back(nullptr);

        return this->_pointers.data();
    }
};

static CommandLineParser make_parser()
{
    CommandLineParser parser;
    parser.add_argument("str_arg", ArgType_String);
    parser.add_argument("int_arg", ArgType_Int);
    parser.add_argument("bool_arg", ArgType_Bool);
    parser.add_argument("bool_store_true", ArgType_Bool, ArgMode_StoreTrue);
    parser.add_argument("bool_store_false", ArgType_Bool, ArgMode_StoreFalse);
    parser.add_argument("another-int-arg", ArgType_Int);
    parser.add_argument("output", ArgType_String, ArgMode_Store, "o");
    return parser;
}

STDROMANO_TEST_CASE(all_assignment_styles)
{
    CommandLineParser parser = make_parser();

    Argv args = {"--str_arg=arg string value",
                 "--int_arg=573849",
                 "--bool_arg:False",
                 "--bool_store_true",
                 "-another-int-arg=-47381",
                 "--",
                 "a_command_after_args",
                 "--with-flags"};

    STDROMANO_REQUIRE(parser.parse(args.argc(), args.argv()).has_value());

    STDROMANO_CHECK_EQ(parser.get_argument_value<StringD>("str_arg"), StringD("arg string value"));
    STDROMANO_CHECK_EQ(parser.get_argument_value<int>("int_arg"), 573849);
    STDROMANO_CHECK(!parser.get_argument_value<bool>("bool_arg"));
    STDROMANO_CHECK(parser.get_argument_value<bool>("bool_store_true"));
    STDROMANO_CHECK_EQ(parser.get_argument_value<int>("another-int-arg"), -47381);
    STDROMANO_CHECK(parser.has_command_after_args());
    STDROMANO_CHECK_EQ(parser.get_command_after_args(), StringD("a_command_after_args --with-flags"));
}

STDROMANO_TEST_CASE(space_separated_value)
{
    CommandLineParser parser = make_parser();

    Argv args = {"--int_arg", "42", "--str_arg", "hello"};

    STDROMANO_REQUIRE(parser.parse(args.argc(), args.argv()).has_value());
    STDROMANO_CHECK_EQ(parser.get_argument_value<int>("int_arg"), 42);
    STDROMANO_CHECK_EQ(parser.get_argument_value<StringD>("str_arg"), StringD("hello"));
    STDROMANO_CHECK(!parser.has_command_after_args());
}

STDROMANO_TEST_CASE(quoted_inline_value)
{
    CommandLineParser parser = make_parser();

    Argv args = {"--str_arg='quoted value'"};

    STDROMANO_REQUIRE(parser.parse(args.argc(), args.argv()).has_value());
    STDROMANO_CHECK_EQ(parser.get_argument_value<StringD>("str_arg"), StringD("quoted value"));
}

STDROMANO_TEST_CASE(unterminated_quote_is_an_error)
{
    CommandLineParser parser = make_parser();

    Argv args = {"--str_arg=\"never closed"};

    STDROMANO_CHECK(parser.parse(args.argc(), args.argv()).has_error());
}

STDROMANO_TEST_CASE(missing_value_is_an_error)
{
    CommandLineParser parser = make_parser();

    Argv args = {"--int_arg"};

    STDROMANO_CHECK(parser.parse(args.argc(), args.argv()).has_error());
}

STDROMANO_TEST_CASE(store_false_and_unknown_arguments)
{
    CommandLineParser parser = make_parser();

    Argv args = {"--unknown=1", "positional", "--bool_store_false"};

    STDROMANO_REQUIRE(parser.parse(args.argc(), args.argv()).has_value());
    STDROMANO_CHECK(!parser.get_argument_value<bool>("bool_store_false"));
    STDROMANO_CHECK(parser.has_parsed_argument("bool_store_false"));
    STDROMANO_CHECK(!parser.has_parsed_argument("int_arg"));
}

STDROMANO_TEST_CASE(duplicate_declaration_is_an_error)
{
    CommandLineParser parser = make_parser();

    STDROMANO_CHECK(parser.add_argument("int_arg", ArgType_Int).has_error());
}

STDROMANO_TEST_CASE(alias_sets_the_argument)
{
    CommandLineParser parser = make_parser();

    Argv args = {"-o", "out.exr"};

    STDROMANO_REQUIRE(parser.parse(args.argc(), args.argv()).has_value());
    STDROMANO_CHECK(parser.has_parsed_argument("output"));
    STDROMANO_CHECK_EQ(parser.get_argument_value<StringD>("output"), StringD("out.exr"));
}

STDROMANO_TEST_CASE(fuzz_generated_command_lines_round_trip)
{
    const auto report = fuzz::run_property(fixtures::options("cmdline_round_trip", 500), [](fuzz::Source& source) {
        CommandLineParser parser = make_parser();

        std::vector<std::vector<std::string>> groups;

        const long long int_value = source.range<long long>(-1000000, 1000000);
        const StringD raw = source.string(16, "abcdefXYZ0123 _./");
        const std::string str_value = std::string("v") + std::string(raw.c_str(), raw.size());
        const bool flag = source.boolean();

        switch(source.index(3))
        {
            case 0:
                groups.push_back({"--int_arg=" + std::to_string(int_value)});
                break;
            case 1:
                groups.push_back({"--int_arg:" + std::to_string(int_value)});
                break;
            default:
                groups.push_back({"--int_arg", std::to_string(int_value)});
                break;
        }

        if(source.boolean())
            groups.push_back({"--str_arg", str_value});
        else
            groups.push_back({"--str_arg=" + str_value});

        if(flag)
            groups.push_back({"--bool_store_true"});

        const std::size_t noise = source.range<std::size_t>(0, 4);

        for(std::size_t i = 0; i < noise; ++i)
            groups.insert(groups.begin() + static_cast<std::ptrdiff_t>(source.index(groups.size() + 1)), {"positional"});

        std::vector<std::string> args;

        for(const auto& group : groups)
            args.insert(args.end(), group.begin(), group.end());

        std::string tail;

        if(source.boolean())
        {
            args.push_back("--");
            args.push_back("tail");
            args.push_back("--int_arg=0");
            tail = "tail --int_arg=0";
        }

        Argv argv(args);

        STDROMANO_FUZZ_CHECK(parser.parse(argv.argc(), argv.argv()).has_value());
        STDROMANO_FUZZ_CHECK_EQ(parser.get_argument_value<long long>("int_arg"), int_value);

        const StringD parsed = parser.get_argument_value<StringD>("str_arg");
        STDROMANO_FUZZ_CHECK_EQ(std::string(parsed.c_str(), parsed.size()), str_value);
        STDROMANO_FUZZ_CHECK_EQ(parser.get_argument_value<bool>("bool_store_true"), flag);

        const StringD after = parser.get_command_after_args();
        STDROMANO_FUZZ_CHECK_EQ(std::string(after.c_str(), after.size()), tail);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_arbitrary_arguments_do_not_crash)
{
    const auto report = fuzz::run_property(fixtures::options("cmdline_arbitrary", 1000), [](fuzz::Source& source) {
        CommandLineParser parser = make_parser();

        std::vector<std::string> args;

        const std::size_t count = source.size(8);

        for(std::size_t i = 0; i < count; ++i)
        {
            const StringD arg = source.string(24, "-=:'\"abcdefinostr_0123 ");
            args.emplace_back(arg.c_str(), arg.size());
        }

        Argv argv(args);
        (void)parser.parse(argv.argc(), argv.argv());

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
