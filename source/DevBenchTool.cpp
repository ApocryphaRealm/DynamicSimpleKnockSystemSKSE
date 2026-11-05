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

		void ControlTool(void*, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			const std::string_view args = a_argsJson ? a_argsJson : "";
			std::string out;
			if (args.find("\"check\"") != std::string_view::npos)
			{
				const auto id = RefFrom(args);
				out = OnMain([id]() { return knock::CheckJson(id); });
			}
			else if (args.find("\"knock\"") != std::string_view::npos)
			{
				const auto id = RefFrom(args);
				out = OnMain([id]() { return knock::KnockJson(id); });
			}
			else if (args.find("\"box\"") != std::string_view::npos)
			{
				const auto id = RefFrom(args);
				out = OnMain([id]() { return knock::ChoiceJson(id); });
			}
			else if (args.find("\"endletin\"") != std::string_view::npos)
			{
				out = OnMain([]() { return knock::EndLetInJson(); });
			}
			else if (args.find("\"reload\"") != std::string_view::npos)
			{
				out = OnMain([]() {
					const bool ok = settings::Reload();
					knock::ResolveExclusions();
					knock::RefreshPrompt();
					return std::format(R"({{"ok":{},"op":"reload"}})", ok ? "true" : "false");
				});
			}
			else if (args.find("\"strings\"") != std::string_view::npos)
			{
				out = std::format(R"({{"ok":true,"op":"strings","strings":{}}})", strings::StatusJson());
			}
			else
			{
				out = OnMain([]() { return knock::StateJson(); });
			}
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
			"\"description\":\"Dynamic Simple Knock System SKSE live state and driver. No op: settings, hour, prompt door, the door let open, counters, the last events. "
			"op=check ref=<hex>: the rule's verdict on a door and the residents found. op=box ref=<hex>: opens the choice box for that door. op=knock ref=<hex>: the box's Knock button. "
			"op=endletin ends the let-in now. op=reload re-reads the INI. op=strings reports the language.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{\"op\":{\"type\":\"string\"},\"ref\":{\"type\":\"string\"}}},"
			"\"readOnly\":false"
			"}";

		if (devBench->RegisterTool("dsk.control", descriptor, &ControlTool, nullptr))
		{
			logger::info("Registered \"dsk.control\" with DevBench (build {})", devBench->GetBuildNumber());
			registered = true;
		}
	}
}
