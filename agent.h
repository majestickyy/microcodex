// SPDX-FileCopyrightText: 2026 Paolo Anzani
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "api.h"
#include "subagent.h"

#include <memory>
#include <string>
#include <vector>

namespace microcodex {

    // Creates the filesystem and command tools used by the first coding agent.
    // The tool definitions are immutable. Callers should still avoid
    // overlapping filesystem mutations when executing their calls in parallel.
    std::vector<std::shared_ptr<const ToolBase>> makeCodingTools();

    // Overload that additionally advertises the "task" delegation tool. The
    // tool is omitted when subagent.runner is empty.
    std::vector<std::shared_ptr<const ToolBase>> makeCodingTools(SubAgentOptions subagent);

    // Provides the model instructions and tool set common to the terminal
    // agent. Authentication is deliberately left to the caller.
    CodexApiConfig makeCodingAgentConfig(std::string model);

} // namespace microcodex
