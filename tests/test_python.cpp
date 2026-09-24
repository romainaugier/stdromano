// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.hpp"
#include "stdromano/python.hpp"

#include "fixtures.hpp"

#include <string>

using namespace stdromano;

static std::size_t count_nodes(Python::AST& ast)
{
    std::size_t count = 0;

    Python::visit(ast.root(), [&](Python::Node*, std::uint32_t) {
        ++count;
        return true;
    });

    return count;
}

static bool parses(const char* source)
{
    Python::AST ast;
    return ast.from_text(StringD(source)) && ast.root() != nullptr;
}

STDROMANO_TEST_CASE(parses_statements)
{
    const char* sources[] = {
        "x = 1\n",
        "x = 1 + 2 * 3 - (4 / 5)\n",
        "a, b = b, a\n",
        "if x:\n    y = 1\nelif z:\n    y = 2\nelse:\n    y = 3\n",
        "while i < 10:\n    i += 1\n",
        "for item in items:\n    print(item)\n",
        "def add(a, b=2):\n    return a + b\n",
        "class Point:\n    def __init__(self, x):\n        self.x = x\n",
        "values = [1, 2, 3]\nmapping = {'a': 1}\n",
        "import os\nfrom sys import path\n",
        "result = obj.method(1, key=2)[0]\n",
    };

    for(const char* source : sources)
        STDROMANO_CHECK_MSG(parses(source), source);
}

STDROMANO_TEST_CASE(tree_has_nodes)
{
    Python::AST ast;

    STDROMANO_REQUIRE(ast.from_text(StringD("def f(x):\n    return x * 2\n\ny = f(3)\n")));
    STDROMANO_REQUIRE(ast.root() != nullptr);
    STDROMANO_CHECK_GT(count_nodes(ast), 4u);
}

STDROMANO_TEST_CASE(data_file_when_present)
{
    const StringD path(TESTS_DATA_DIR "/python/test_all.py");

    if(!fs::path_exists(path))
        return;

    const auto source = fs::load_file_content(path);
    STDROMANO_REQUIRE(source.has_value());

    Python::AST ast;
    STDROMANO_CHECK(ast.from_text(source.value()));
}

STDROMANO_TEST_CASE(fuzz_arbitrary_source_does_not_crash)
{
    fixtures::QuietLogs quiet;

    auto options = fixtures::options("python_arbitrary_source", 2000);
    options.max_input_size = 160;
    options.dictionary.tokens = {"def ", "class ", "if ", "elif ", "else:", "while ", "for ", " in ", "return ",
                                 "import ", "from ", "lambda ", ":", "\n", "    ", "(", ")", "[", "]", "{",
                                 "}", ",", "=", "+=", "**", "'", "\"", "\"\"\"", "#", "\\", "1.5e3", "x"};
    options.corpus = {{'x', ' ', '=', ' ', '1', '\n'},
                      {'d', 'e', 'f', ' ', 'f', '(', ')', ':', '\n', ' ', ' ', ' ', ' ', 'p', 'a', 's', 's', '\n'}};

    const auto report = fuzz::run_input(options, [](const std::uint8_t* data, std::size_t size) {
        std::string source(reinterpret_cast<const char*>(data), size);

        for(char& c : source)
            if(c == '\0')
                c = ' ';

        Python::AST ast;
        (void)ast.from_text(StringD::make_from_c_str(source.c_str(), source.size()));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
