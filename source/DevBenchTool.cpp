#include "PCH.h"

#include "DevBenchTool.h"

#include "DevBench/DevBenchAPI.h"
#include "Knock.h"
#include "Settings.h"
#include "utils/Logger.h"
#include "utils/Strings.h"

#include <chrono>
#include <cstdlib>
#include <format>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace DevBenchTool
{
	namespace
	{
		// DevBench calls on its listener thread; game state is read on the main thread.
		template <class F>
		std::string OnMain(F a_fn)
		{
			auto* tasks = SKSE::GetTaskInterface();
			if (!tasks) { return R"({"ok":false,"error":"no task interface"})"; }
			auto promise = std::make_shared<std::promise<std::string>>();
			auto future = promise->get_future();
			tasks->AddTask([promise, a_fn]() { promise->set_value(a_fn()); });
			if (future.wait_for(std::chrono::seconds(5)) != std::future_status::ready) { return R"({"ok":false,"error":"the main thread did not answer in 5 seconds"})"; }
			return future.get();
		}

		std::uint32_t RefFrom(std::string_view a_args)
		{
			const auto at = a_args.find("\"ref\"");
			if (at == std::string_view::npos) { return 0; }
			const auto start = a_args.find_first_of("0123456789abcdefABCDEF", at + 5);
			if (start == std::string_view::npos) { return 0; }
			return static_cast<std::uint32_t>(std::strtoul(std::string(a_args.substr(start, 12)).c_str(), nullptr, 16));
		}

		// The op's name: the string value of "op".
		std::string OpFrom(std::string_view a_args)
		{
			const auto at = a_args.find("\"op\"");
			if (at == std::string_view::npos) { return {}; }
			const auto open = a_args.find('"', a_args.find(':', at + 4) + 1);
			if (open == std::string_view::npos) { return {}; }
			const auto close = a_args.find('"', open + 1);
			if (close == std::string_view::npos) { return {}; }
			return std::string(a_args.substr(open + 1, close - open - 1));
		}

		// A number field ("speech":15, "speech":"15") or a flag ("night":true / false / 1 / 0). Missing: nothing.
		std::optional<int> FieldFrom(std::string_view a_args, const char* a_name)
		{
			const std::string key = std::string("\"") + a_name + "\"";
			const auto at = a_args.find(key);
			if (at == std::string_view::npos) { return std::nullopt; }
			auto p = a_args.find(':', at + key.size());
			if (p == std::string_view::npos) { return std::nullopt; }
			++p;
			while (p < a_args.size() && (a_args[p] == ' ' || a_args[p] == '"')) { ++p; }
			if (a_args.substr(p, 4) == "true") { return 1; }
			if (a_args.substr(p, 5) == "false") { return 0; }
			if (a_args.substr(p, 4) == "null") { return std::nullopt; }
			char* end = nullptr;
			const std::string rest(a_args.substr(p, 16));
			const long v = std::strtol(rest.c_str(), &end, 10);
			if (end == rest.c_str()) { return std::nullopt; }
			return static_cast<int>(v);
		}

		void ControlTool(void*, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			const std::string args = a_argsJson ? a_argsJson : "";
			const std::string op = OpFrom(args);
			const auto id = RefFrom(args);
			std::string out;
			if (op == "check") { out = OnMain([id]() { return knock::CheckJson(id); }); }
			else if (op == "knock" || op == "persuade" || op == "intimidate") { out = OnMain([id, op]() { return knock::ActJson(id, op.c_str()); }); }
			else if (op == "box") { out = OnMain([id]() { return knock::ChoiceJson(id, false); }); }
			else if (op == "refusalbox") { out = OnMain([id]() { return knock::ChoiceJson(id, true); }); }
			else if (op == "set") { out = OnMain([args]() { return knock::SetJson([&args](const char* a_name) { return FieldFrom(args, a_name); }, false); }); }
			else if (op == "clear") { out = OnMain([]() { return knock::SetJson([](const char*) { return std::optional<int>{}; }, true); }); }
			else if (op == "endletin") { out = OnMain([]() { return knock::EndLetInJson(); }); }
			else if (op == "reload")
			{
				out = OnMain([]() {
					const bool ok = settings::Reload();
					knock::ResolveExclusions();
					knock::RefreshPrompt();
					return std::format(R"({{"ok":{},"op":"reload"}})", ok ? "true" : "false");
				});
			}
			else if (op == "strings") { out = std::format(R"({{"ok":true,"op":"strings","strings":{}}})", strings::StatusJson()); }
			else { out = OnMain([]() { return knock::StateJson(); }); }
			a_write(a_sink, out.c_str());
		}
	}

	void Init(bool a_lastAttempt)
	{
		static bool registered = false;
		if (registered) { return; }

		DevBenchAPI::IDevBenchInterface001* devBench = DevBenchAPI::GetDevBenchInterface001();
		if (!devBench)
		{
			if (a_lastAttempt) { logger::info("DevBench not detected; skipping the \"dsk.control\" tool"); }
			else { logger::debug("DevBench not detected yet; will retry at the next message"); }
			return;
		}

		constexpr const char* descriptor =
			"{"
			"\"description\":\"Dynamic Simple Knock System SKSE live state and driver. No op: settings, hour, prompt door, the let-in, the refusal, overrides, the last decision, counters, the last events. "
			"op=check ref=<hex>: the rule's verdict on a door, the residents found and the decision it would make now (inputs and result). "
			"op=knock / op=persuade / op=intimidate ref=<hex>: that button, without the box; answers with the decision. op=box ref=<hex>: opens the knock box (the refusal box while a refusal stands). op=refusalbox ref=<hex>: the refusal box. "
			"op=set with any of speech, rank (-4..4), playerLevel, theirLevel, confidence (0..4), bounty, stolen, night (true/false), asleep (true/false): replaces those decision inputs until op=clear or a game load; resetRefusal=true forgets the refusal. "
			"op=endletin ends the let-in now. op=reload re-reads the INI. op=strings reports the language.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{\"op\":{\"type\":\"string\"},\"ref\":{\"type\":\"string\"}},\"additionalProperties\":true},"
			"\"readOnly\":false"
			"}";

		if (devBench->RegisterTool("dsk.control", descriptor, &ControlTool, nullptr))
		{
			logger::info("Registered \"dsk.control\" with DevBench (build {})", devBench->GetBuildNumber());
			registered = true;
		}
	}
}
