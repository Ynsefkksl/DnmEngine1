#pragma once

#include <filesystem>
#include <fstream>
#include <print>
#include <string>
#include <string_view>
#include <format>
#include <source_location>

#include "Utility/EngineDefines.hpp"

#define LogSource std::source_location::current()

class ENGINE_API Log {
	enum class MessageType : uint8_t {
		eError = 31,//ansi color
		eWarning = 33,//ansi color
		eInfo = 37//ansi color
	};
public:
	static Log& Get() {
		static Log instance;
		return instance;
	}

	Log() = default;

	Log(Log&&) = delete;
	Log& operator=(Log&&) = delete;
	Log(Log&) = delete;
	Log& operator=(Log&) = delete;

	void Init(const std::filesystem::path& path);
	void CleanUp();

	template <class... Types>
	// source_location is placed first to avoid conflict with the args parameters
	void Info(const std::source_location& loc, const std::format_string<Types...> fmt, Types&&... args);

	template <class... Types>
	// source_location is placed first to avoid conflict with the args parameters
	void Warning(const std::source_location& loc, const std::format_string<Types...> fmt, Types&&... args);

	template <class... Types>
	// source_location is placed first to avoid conflict with the args parameters
	void Error(const std::source_location& loc, const std::format_string<Types...> fmt, Types&&... args);
private:
	void BeginHtml();
	void EndHtml();
	void WriteHtml(std::string_view message, MessageType color);

	static void MessageBoxError(std::string_view message);

	const std::string_view ansiReset = "\033[0m";
	std::filesystem::path m_path;

	std::ofstream logFile{};
};

inline void Log::Init(const std::filesystem::path& path) {
	if (!path.has_relative_path()) {
		std::println("{} this path is not valid", path.string());
		return;
	}

	m_path = path/"Log.html";

	BeginHtml();
}

inline void Log::CleanUp() {
	EndHtml();
}

template <class... Types>
inline void Log::Info(const std::source_location& loc, const std::format_string<Types...> fmt, Types&&... args) {
	std::string message = "[INFO] ";
	message += "[" + std::string(loc.function_name()) + "] ";
	message += ": ";
	message += std::vformat(fmt.get(), std::make_format_args(args...));
	std::println("\033[{}m{}\033[0m", static_cast<uint32_t>(MessageType::eInfo), message);
	WriteHtml(message, MessageType::eInfo);
}

template <class... Types>
inline void Log::Warning(const std::source_location& loc, const std::format_string<Types...> fmt, Types&&... args) {
	std::string message = "[WARNING] ";
	message += "[" + std::string(loc.function_name()) + "] ";
	message += ": ";
	message += std::vformat(fmt.get(), std::make_format_args(args...));
	std::println("\033[{}m{}\033[0m", static_cast<uint32_t>(MessageType::eWarning), message);
	WriteHtml(message, MessageType::eWarning);
}

template <class... Types>
inline void Log::Error(const std::source_location& loc, const std::format_string<Types...> fmt, Types&&... args) {
	std::string message = "[ERROR] ";
	message += "[" + std::string(loc.function_name()) + "] ";
	message += ": ";
	message += std::vformat(fmt.get(), std::make_format_args(args...));
	std::println("\033[{}m{}\033[0m", static_cast<uint32_t>(MessageType::eError), message);
	WriteHtml(message, MessageType::eError);
	MessageBoxError(message);
	exit(1);
}

#define LogInfo(fmt, ...) Log::Get().Info(LogSource, fmt, __VA_ARGS__)
#define LogWarning(fmt, ...) Log::Get().Warning(LogSource, fmt, __VA_ARGS__)
#define LogError(fmt, ...) Log::Get().Error(LogSource, fmt, __VA_ARGS__)