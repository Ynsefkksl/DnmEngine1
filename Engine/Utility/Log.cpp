#include "Utility/Log.hpp"

#define LEAN_AND_MEAN
#include "windows.h"

void Log::BeginHtml() {
	logFile.open(m_path);
	if (!logFile.is_open()) {
		std::println("log file don't be opened");
		return;
	}

	logFile << R"(
	<!DOCTYPE html>
	<html lang="en"><head><meta charset="UTF-8"><title>Dnm Engine Log File</title></head>
	<body style="background-color: #222222;">
	<style>
	  .Error {
		color: #ff5555; font-size:x-large; font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
	  }
	  .Warning {
		color: #eeff55; font-size:x-large; font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
	  }
	  .Info {
		color: #eeeeee; font-size:x-large; font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
	  }
	</style>
	)";
	logFile.close();
}

void Log::EndHtml() {
	logFile.open(m_path, std::ios::app);
	if (!logFile.is_open()) {
		std::println("log file don't be opened");
		return;
	}

	logFile << "</body>\n</html>";

	logFile.close();
}

void Log::WriteHtml(const std::string_view message, const MessageType color) {
	logFile.open(m_path, std::ios::app);
	if (!logFile.is_open()) {
		std::println("log file don't be opened");
		return;
	}

	std::string colorStr = "";
	switch (color) {
	case MessageType::eError:
		colorStr = "Error";
		break;
	case MessageType::eWarning:
		colorStr = "Warning";
		break;
	case MessageType::eInfo:
		colorStr = "Info";
		break;
	default:
		colorStr = "Info";
		break;
	}

	logFile << "<p class = " << colorStr << ">" << std::string(message) + "</p>\n";

	logFile.close();
}

void Log::MessageBoxError(const std::string_view message) {
	MessageBoxA(GetActiveWindow(), message.data(), "Error", MB_OK | MB_ICONERROR);
	throw std::runtime_error("");
}