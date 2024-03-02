#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

#include "global.hh"


class Progress
{
public:
	enum class Stage
	{
		LOADING,
		SOLVING,
		OPTIMIZING,
	};
	
	using steps_t = std::uintmax_t;
	using completion_t = double;
	using calcStepCompletion_t = std::function<completion_t()>;
	
	static calcStepCompletion_t calc0StepCompletion;
	
private:
	static constexpr std::size_t STAGE_COUNT = 3;
	static std::uint_fast8_t stageCounters[STAGE_COUNT];
	
	using timePoint_t = std::chrono::steady_clock::time_point;
	
	static constexpr double reportInterval = 1.0;
	
	static Progress *progress;
	static timePoint_t programStartTime;
	
	const Stage stage;
	const char *const processName;
	timePoint_t startTime, stepStartTime, lastReportTime;
	steps_t allSteps;
	steps_t stepsSoFar = 0;
	steps_t substepsSoFar, substepsToSkip;
	completion_t stepCompletion;
	bool relatedSteps;
	bool visible;
	std::vector<const char*> infoTexts;
	std::uint_fast8_t reportLines = 0;
	
	steps_t calcStepsToSkip(const double secondsSinceStepStart, const double secondsToSkip, const double secondsPerStep) const;
	static double getSecondsSinceStart(const timePoint_t currentTime);
	static bool checkProgramRunTime(const timePoint_t currentTime);
	bool checkReportInterval(const bool force);
	static void printTime(const double seconds);
	void clearReport(const bool clearStage);
	void reportStage() const;
	static void reportSingleLevel(const bool indentMore, const completion_t completion, const double et, const double eta);
	void reportProgress();
	void reportProgress(const calcStepCompletion_t &calcStepCompletion) { stepCompletion = calcStepCompletion(); return reportProgress(); }
	void handleStep(const calcStepCompletion_t &calcStepCompletion, const bool force) { if (checkReportInterval(force)) reportProgress(calcStepCompletion); }
	
public:
	static void init();
	
	class CerrGuard
	{
		Progress *const progress;
		const bool reportedBefore;
		CerrGuard(Progress *const progress) : progress(progress), reportedBefore(progress != nullptr && progress->reportLines != 0) { if (progress != nullptr) { progress->clearReport(true); std::clog << std::flush; } }
		friend class Progress;
	public:
		CerrGuard(const CerrGuard&) = delete;
		CerrGuard& operator=(const CerrGuard&) = delete;
		~CerrGuard() { if (reportedBefore) progress->reportProgress(); }
		template<typename T>
		const CerrGuard& operator<<(T val) const { std::cerr << std::forward<T>(val); return *this; }
	};
	
	class InfoGuard
	{
		Progress &progress;
		InfoGuard(Progress &progress) : progress(progress) {}
		friend class Progress;
	public:
		InfoGuard(const InfoGuard&) = delete;
		InfoGuard& operator=(const InfoGuard&) = delete;
		~InfoGuard() { progress.infoTexts.pop_back(); }
	};
	
	template<typename T = std::size_t>
	class CountingStepHelper
	{
		Progress &progress;
		T i = 0;
		const calcStepCompletion_t calcCompletion;
		template<typename T2>
		CountingStepHelper(Progress &progress, const T2 n) : progress(progress), calcCompletion([&i = std::as_const(i), n = static_cast<completion_t>(n)](){ return static_cast<completion_t>(i) / n; }) {}
		friend class Progress;
	public:
		CountingStepHelper(const CountingStepHelper&) = delete;
		CountingStepHelper& operator=(const CountingStepHelper&) = delete;
		void substep(const bool force = false) { progress.substep(calcCompletion, force); ++i; }
		void substep(const T increment, const bool force = false) { progress.substep(calcCompletion, force); i += increment; }
	};
	
	Progress(const Stage stage, const char processName[], const steps_t allSteps, const bool relatedSteps = true, const bool visible = true);
	Progress(const Progress&) = delete;
	Progress& operator=(const Progress&) = delete;
	~Progress() { clearReport(true); progress = nullptr; }
	
	[[nodiscard]] bool isVisible() const { return visible; }
	[[nodiscard]] static CerrGuard cerr() { return {progress}; }
	
	void step(const bool force = false);
	void substep(const calcStepCompletion_t &calcStepCompletion, const bool force = false) { if (visible) { if (--substepsToSkip == 0 || force) [[unlikely]] handleStep(calcStepCompletion, force); ++substepsSoFar; } }
	void substep(const completion_t &completion, const bool force = false) { return substep([completion](){ return completion; }, force); }
	template<typename T = std::size_t, typename T2 = T>
	[[nodiscard]] CountingStepHelper<T> makeCountingStepHelper(const T2 n) { return {*this, n}; }
	
	[[nodiscard]] InfoGuard addInfo(const char infoText[]) { infoTexts.push_back(infoText); return {*this}; }
	
	static void reportTime(const std::string_view eventName);
};
