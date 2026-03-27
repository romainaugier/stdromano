// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/json.hpp"
#include "stdromano/filesystem.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

int main()
{
    spdlog::set_level(spdlog::level::debug);

    stdromano::Json json;

    for(const auto file : { "twitter", "citm_catalog", "canada" })
    {
        SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, json_loadf);
        json.loadf(stdromano::StringD("{}/json/{}.json", TESTS_DATA_DIR, file));
        SCOPED_PROFILE_STOP(json_loadf);

        SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, json_dumpf);
        json.dumpf(2, stdromano::StringD("{}/json/out_{}.json", TESTS_DATA_DIR, file));
        SCOPED_PROFILE_STOP(json_dumpf);
    }

    return 0;
}
