// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/json.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/filesystem.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

int main()
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    stdromano::Json json;
    
    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, json_loadf_canada);
    json.loadf(stdromano::StringD("{}/json/canada.json", TESTS_DATA_DIR));
    SCOPED_PROFILE_STOP(json_loadf_canada);

    return 0;
}