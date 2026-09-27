// SPDX-FileCopyrightText: 2026 Paolo Anzani
// SPDX-License-Identifier: Apache-2.0

#include "subagent.h"

#include <future>
#include <thread>
#include <utility>

namespace microcodex {
    namespace {

        // Runs the delegation on its own thread so the timeout is real: a
        // wedged sub-agent (or a hung network call inside it) cannot pin the
        // parent turn forever. The parent stop_token is forwarded so cancelling
        // the parent turn also abandons the sub-agent.
        class SubAgentTool final : public ToolBase {
        public:
            explicit SubAgentTool(SubAgentOptions options) : options_{std::move(options)} {}

            std::string_view name() const override { return "task"; }

            std::string toJsonString() const override {
                return detail::toolJsonString(
                    name(),
                    "Delegate a self-contained task to a fresh sub-agent and return its result. "
                    "The sub-agent cannot delegate further, so a task is always handled by exactly one agent. "
                    "Use it for work that is independent of the current turn, and pass a prompt that fully "
                    "states the goal, the relevant context, and the expected output format.",
                    R"({"type":"object","properties":{"task":{"type":"string"},"description":{"type":"string"}},"required":["task"],"additionalProperties":false})");
            }

            std::expected<ToolResult, std::string> executeJson(const std::string_view arguments, const std::stop_token stop_token = {}) const override {
                if (!options_.runner) {
                    return std::unexpected("Sub-agent execution is unavailable");
                }

                ToolArguments parsed{arguments};
                auto task = parsed.string("task");
                if (!task) {
                    return std::unexpected(task.error());
                }
                if (task->empty()) {
                    return std::unexpected("Argument 'task' must not be empty");
                }
                // description is a short label for logs and transcripts; it is
                // optional because the task prompt alone is enough to delegate.
                auto description = parsed.string("description");
                if (!description) {
                    description = std::string{};
                }

                if (stop_token.stop_requested()) {
                    return std::unexpected("Tool execution interrupted");
                }

                // The result lives in a shared_ptr, not a stack local: on
                // timeout the sub-agent is abandoned but its thread keeps
                // running and will still write here. A stack local would be
                // written to after this frame is gone.
                struct Outcome {
                    std::string result;
                    std::string failure;
                };
                auto outcome = std::make_shared<Outcome>();
                auto work = std::async(std::launch::async, [this, task = *task, description = *description, outcome]() {
                    try {
                        outcome->result = options_.runner(task, description);
                    } catch (const std::exception &error) {
                        outcome->failure = error.what();
                    } catch (...) {
                        outcome->failure = "sub-agent failed for an unknown reason";
                    }
                });

                // A timed-out std::async cannot be cancelled, so the abandoned
                // thread is simply left to finish against the shared outcome.
                if (work.wait_for(options_.timeout) != std::future_status::ready) {
                    return std::unexpected("Sub-agent timed out after " +
                                           std::to_string(options_.timeout.count()) + "s");
                }
                work.get();

                if (!outcome->failure.empty()) {
                    return std::unexpected("Sub-agent failed: " + outcome->failure);
                }
                if (outcome->result.empty()) {
                    return std::unexpected("Sub-agent returned no result");
                }

                return ToolResult{std::move(outcome->result), {}};
            }

        private:
            SubAgentOptions options_;
        };

    } // namespace

    std::shared_ptr<const ToolBase> makeSubAgentTool(SubAgentOptions options) {
        return std::make_shared<SubAgentTool>(std::move(options));
    }

} // namespace microcodex
