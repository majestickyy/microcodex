// SPDX-FileCopyrightText: 2026 Paolo Anzani
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tool.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace microcodex {

    // Runs one complete sub-agent turn and returns its final assistant text.
    // The tool owns no model state: the runner is supplied by the caller so the
    // tool can be exercised without a network or credentials.
    using SubAgentRunner = std::function<std::string(std::string_view task, std::string_view description)>;

    struct SubAgentOptions {
        SubAgentRunner runner;
        // Bounds a single delegation. A sub-agent that exceeds this returns a
        // tool error to the parent turn instead of blocking the parent forever.
        std::chrono::seconds timeout{600};
    };

    // Creates the "task" tool: delegates a self-contained prompt to a fresh
    // sub-agent and returns its result to the calling turn.
    //
    // The sub-agent is deliberately not given this tool, so a delegated task
    // cannot fan out into unbounded nesting.
    std::shared_ptr<const ToolBase> makeSubAgentTool(SubAgentOptions options);

} // namespace microcodex
