// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.hpp"
#include "stdromano/json.hpp"

#include "fixtures.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

using namespace stdromano;

struct Model
{
    enum Kind
    {
        Null,
        Bool,
        U64,
        I64,
        F64,
        Str,
        Array,
        Dict
    };

    Kind kind = Null;
    bool b = false;
    std::uint64_t u = 0;
    std::int64_t i = 0;
    double f = 0.0;
    std::string s;
    std::vector<Model> items;
    std::vector<std::pair<std::string, Model>> entries;
};

static Model random_model(fuzz::Source& source, const std::size_t depth)
{
    Model model;

    switch(source.index(depth == 0 ? 6 : 8))
    {
        case 0:
            model.kind = Model::Null;
            break;
        case 1:
            model.kind = Model::Bool;
            model.b = source.boolean();
            break;
        case 2:
            model.kind = Model::U64;
            model.u = source.integer<std::uint64_t>();
            break;
        case 3:
            model.kind = Model::I64;
            model.i = source.range<std::int64_t>(std::numeric_limits<std::int64_t>::min(), -1);
            break;
        case 4:
            model.kind = Model::F64;
            model.f = source.floating<double>();

            if(!std::isfinite(model.f))
                model.f = 0.5;
            break;
        case 5:
        {
            model.kind = Model::Str;
            const StringD str = source.string(24);
            model.s.assign(str.c_str(), str.size());
            break;
        }
        case 6:
        {
            model.kind = Model::Array;
            const std::size_t count = source.size(6);

            for(std::size_t i = 0; i < count; ++i)
                model.items.push_back(random_model(source, depth - 1));

            break;
        }
        default:
        {
            model.kind = Model::Dict;
            const std::size_t count = source.size(6);

            for(std::size_t i = 0; i < count; ++i)
                model.entries.emplace_back("k" + std::to_string(i), random_model(source, depth - 1));

            break;
        }
    }

    return model;
}

static JsonObject* build(Json& json, const Model& model)
{
    switch(model.kind)
    {
        case Model::Null:
            return json.make_null();
        case Model::Bool:
            return json.make_bool(model.b);
        case Model::U64:
            return json.make_u64(model.u);
        case Model::I64:
            return json.make_i64(model.i);
        case Model::F64:
            return json.make_f64(model.f);
        case Model::Str:
            return json.make_str(model.s.c_str());
        case Model::Array:
        {
            JsonObject* array = json.make_array();

            for(const Model& item : model.items)
                json.array_append(array, build(json, item));

            return array;
        }
        default:
        {
            JsonObject* dict = json.make_dict();

            for(const auto& [key, value] : model.entries)
                json.dict_append(dict, key.c_str(), build(json, value));

            return dict;
        }
    }
}

static bool same_f64(const double lhs, const double rhs)
{
    return lhs == rhs && std::signbit(lhs) == std::signbit(rhs);
}

static bool matches(const JsonObject* object, const Model& model)
{
    if(object == nullptr)
        return false;

    switch(model.kind)
    {
        case Model::Null:
            return object->is_null();
        case Model::Bool:
            return object->is_bool() && object->get_bool() == model.b;
        case Model::U64:
            return object->is_u64() && object->get_u64() == model.u;
        case Model::I64:
            return object->is_i64() && object->get_i64() == model.i;
        case Model::F64:
            return object->is_f64() && same_f64(object->get_f64(), model.f);
        case Model::Str:
            return object->is_str() && object->get_str_size() == model.s.size() &&
                   std::memcmp(object->get_str(), model.s.data(), model.s.size()) == 0;
        case Model::Array:
        {
            if(!object->is_array() || object->array_size() != model.items.size())
                return false;

            std::size_t index = 0;

            for(const JsonObject* item : object->array_items())
                if(!matches(item, model.items[index++]))
                    return false;

            return index == model.items.size();
        }
        default:
        {
            if(!object->is_dict() || object->dict_size() != model.entries.size())
                return false;

            for(const auto& [key, value] : model.entries)
                if(!matches(object->dict_find(key.c_str()), value))
                    return false;

            return true;
        }
    }
}

static bool parse(Json& json, const char* text)
{
    return json.loads(text, std::strlen(text));
}

static std::string string_of(const JsonObject* object)
{
    return std::string(object->get_str(), object->get_str_size());
}

STDROMANO_TEST_CASE(parses_scalars)
{
    Json json;

    STDROMANO_REQUIRE(parse(json, "null"));
    STDROMANO_CHECK(json.root()->is_null());

    STDROMANO_REQUIRE(parse(json, "true"));
    STDROMANO_CHECK(json.root()->is_bool());
    STDROMANO_CHECK(json.root()->get_bool());

    STDROMANO_REQUIRE(parse(json, "  false  "));
    STDROMANO_CHECK(!json.root()->get_bool());

    STDROMANO_REQUIRE(parse(json, "18446744073709551615"));
    STDROMANO_CHECK(json.root()->is_u64());
    STDROMANO_CHECK_EQ(json.root()->get_u64(), 18446744073709551615ull);

    STDROMANO_REQUIRE(parse(json, "-42"));
    STDROMANO_CHECK(json.root()->is_i64());
    STDROMANO_CHECK_EQ(json.root()->get_i64(), -42);

    STDROMANO_REQUIRE(parse(json, "-1.5e3"));
    STDROMANO_CHECK(json.root()->is_f64());
    STDROMANO_CHECK_EQ(json.root()->get_f64(), -1500.0);
}

STDROMANO_TEST_CASE(parses_numbers_at_the_limits)
{
    Json json;

    STDROMANO_REQUIRE(parse(json, "-9223372036854775808"));
    STDROMANO_CHECK(json.root()->is_i64());
    STDROMANO_CHECK_EQ(json.root()->get_i64(), std::numeric_limits<std::int64_t>::min());

    STDROMANO_REQUIRE(parse(json, "18446744073709551616"));
    STDROMANO_CHECK(json.root()->is_f64());

    STDROMANO_REQUIRE(parse(json, "0.1"));
    STDROMANO_CHECK_EQ(json.root()->get_f64(), 0.1);

    STDROMANO_REQUIRE(parse(json, "[1e-10, 1.7976931348623157e308]"));
    const StringD dumped = json.dumps();

    Json reparsed;
    STDROMANO_REQUIRE(reparsed.loads(dumped.c_str(), dumped.size()));

    std::vector<double> values;

    for(const JsonObject* item : reparsed.root()->array_items())
        values.push_back(item->get_f64());

    STDROMANO_REQUIRE_EQ(values.size(), 2u);
    STDROMANO_CHECK_EQ(values[0], 1e-10);
    STDROMANO_CHECK_EQ(values[1], 1.7976931348623157e308);
}

STDROMANO_TEST_CASE(parses_strings_with_escapes)
{
    Json json;

    STDROMANO_REQUIRE(parse(json, "\"a\\\"b\\\\c\\/d\\n\\t\""));
    STDROMANO_REQUIRE(json.root()->is_str());
    STDROMANO_CHECK_EQ(string_of(json.root()), "a\"b\\c/d\n\t");

    STDROMANO_REQUIRE(parse(json, "\"caf\\u00e9 \\u20ac\""));
    STDROMANO_CHECK_EQ(string_of(json.root()), "caf\xC3\xA9 \xE2\x82\xAC");

    // Keep the JSON Unicode escapes as literal backslash-u sequences.
    STDROMANO_REQUIRE(parse(json, "\"\\ud83d\\ude00 \\u0001\""));
    STDROMANO_CHECK_EQ(string_of(json.root()), "\xF0\x9F\x98\x80 \x01");

    const StringD dumped = json.dumps();

    Json reparsed;
    STDROMANO_REQUIRE(reparsed.loads(dumped.c_str(), dumped.size()));
    STDROMANO_CHECK_EQ(string_of(reparsed.root()), "\xF0\x9F\x98\x80 \x01");
}

STDROMANO_TEST_CASE(parses_nested_containers)
{
    Json json;

    STDROMANO_REQUIRE(parse(json, R"({"a": [1, 2, {"b": null}], "c": {"d": "e"}, "f": []})"));

    const JsonObject* root = json.root();
    STDROMANO_REQUIRE(root->is_dict());
    STDROMANO_CHECK_EQ(root->dict_size(), 3u);

    const JsonObject* a = root->dict_find("a");
    STDROMANO_REQUIRE(a != nullptr);
    STDROMANO_REQUIRE(a->is_array());
    STDROMANO_CHECK_EQ(a->array_size(), 3u);

    const JsonObject* c = root->dict_find("c");
    STDROMANO_REQUIRE(c != nullptr);
    STDROMANO_REQUIRE(c->dict_find("d") != nullptr);
    STDROMANO_CHECK_EQ(string_of(c->dict_find("d")), "e");

    STDROMANO_CHECK_EQ(root->dict_find("f")->array_size(), 0u);
    STDROMANO_CHECK(root->dict_find("missing") == nullptr);
}

STDROMANO_TEST_CASE(rejects_malformed_documents)
{
    for(const char* text : {"", "{", "[1, 2", "{\"a\" 1}", "{\"a\": }", "\"unterminated", "tru", "[1,]x",
                            "{} {}", "\"\\x\"", "\"\\u12\"", "-", "1.", "1e", "\"tab\there\""})
    {
        Json json;
        STDROMANO_CHECK_MSG(!parse(json, text), text);
    }
}

STDROMANO_TEST_CASE(builds_and_mutates_documents)
{
    Json json;

    JsonObject* root = json.make_dict();
    json.set_root(root);

    JsonObject* array = json.make_array();
    json.dict_append(root, "values", array);

    for(std::uint64_t i = 0; i < 5; ++i)
        json.array_append(array, json.make_u64(i));

    json.array_pop(array, 0);
    STDROMANO_CHECK_EQ(array->array_size(), 4u);
    STDROMANO_CHECK_EQ(root->dict_find("values")->array_size(), 4u);

    json.dict_append(root, "name", json.make_str("stdromano"));
    STDROMANO_CHECK_EQ(string_of(root->dict_find("name")), "stdromano");
    json.dict_pop(root, "name");
    STDROMANO_CHECK(root->dict_find("name") == nullptr);

    JsonObject* snapshot = json.make_u64(1);
    json.dict_append(root, "snapshot", snapshot);
    json.set_u64(snapshot, 2);
    STDROMANO_CHECK_EQ(root->dict_find("snapshot")->get_u64(), 1u);

    JsonObject* value = json.make_null();
    json.dict_append(root, "value", value, true);
    json.set_i64(value, -7);
    STDROMANO_CHECK(root->dict_find("value")->is_i64());
    json.set_str(value, "now a string");
    STDROMANO_CHECK(root->dict_find("value")->is_str());

    const StringD dumped = json.dumps();

    Json reparsed;
    STDROMANO_REQUIRE(reparsed.loads(dumped.c_str(), dumped.size()));
    STDROMANO_CHECK_EQ(reparsed.root()->dict_find("values")->array_size(), 4u);
    STDROMANO_CHECK_EQ(reparsed.root()->dict_size(), 3u);
}

STDROMANO_TEST_CASE(non_finite_floats_are_written_as_null)
{
    Json json;
    json.set_root(json.make_f64(std::numeric_limits<double>::infinity()));
    STDROMANO_CHECK_EQ(json.dumps(), StringD("null"));
}

STDROMANO_TEST_CASE(file_round_trip)
{
    Json json;
    STDROMANO_REQUIRE(parse(json, R"({"a": [1, -2, 3.5, "x"], "b": true})"));

    const StringD path = test::temp_path("out.json");
    STDROMANO_REQUIRE(json.dumpf(2, path));

    Json loaded;
    STDROMANO_REQUIRE(loaded.loadf(path));
    STDROMANO_CHECK_EQ(loaded.root()->dict_find("a")->array_size(), 4u);
    STDROMANO_CHECK(loaded.root()->dict_find("b")->get_bool());
}

STDROMANO_TEST_CASE(benchmark_files_when_present)
{
    for(const char* name : {"twitter", "citm_catalog", "canada"})
    {
        const StringD path(TESTS_DATA_DIR "/json/{}.json", name);

        if(!fs::path_exists(path))
            continue;

        Json json;
        STDROMANO_CHECK_MSG(json.loadf(path), path);
    }
}

STDROMANO_TEST_CASE(fuzz_dump_load_round_trip)
{
    const auto report = fuzz::run_property(fixtures::options("json_round_trip", 500), [](fuzz::Source& source) {
        const Model model = random_model(source, 4);

        Json json;
        json.set_root(build(json, model));

        STDROMANO_FUZZ_CHECK(matches(json.root(), model));

        for(const std::size_t indent : {std::size_t(0), std::size_t(2)})
        {
            const StringD dumped = json.dumps(indent);

            Json reparsed;
            STDROMANO_FUZZ_CHECK(reparsed.loads(dumped.c_str(), dumped.size()));
            STDROMANO_FUZZ_CHECK(matches(reparsed.root(), model));
        }

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_arbitrary_input_does_not_crash)
{
    auto options = fixtures::options("json_arbitrary_input", 3000);
    options.max_input_size = 256;
    options.dictionary.tokens = {"{", "}", "[", "]", ":", ",", "\"", "\\u", "\\\"", "\\ud83d", "true", "false",
                                 "null", "-", "1e308", "0.5", "18446744073709551616", "\"key\":"};
    options.corpus = {{'{', '"', 'a', '"', ':', '[', '1', ',', '2', ']', '}'},
                      {'[', 'n', 'u', 'l', 'l', ',', 't', 'r', 'u', 'e', ']'},
                      {'"', '\\', 'u', '0', '0', 'e', '9', '"'}};

    const auto report = fuzz::run_input(options, [](const std::uint8_t* data, std::size_t size) {
        Json json;

        if(json.loads(reinterpret_cast<const char*>(data), size))
        {
            const StringD dumped = json.dumps();

            Json reparsed;
            STDROMANO_FUZZ_CHECK(reparsed.loads(dumped.c_str(), dumped.size()));
            STDROMANO_FUZZ_CHECK(reparsed.dumps() == dumped);
        }

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
