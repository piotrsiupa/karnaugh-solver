#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <iostream>
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
	using calcSubstepCompletion_t = std::function<completion_t()>;
	
	static calcSubstepCompletion_t calc0SubstepCompletion;
	
private:
	static constexpr std::size_t STAGE_COUNT = 3;
	static std::uint_fast8_t stageCounters[STAGE_COUNT];
	
	using timePoint_t = std::chrono::steady_clock::time_point;
	
	static constexpr double reportInterval = 1.0;
	
	const Stage stage;
	const char *const processName;
	timePoint_t startTime, lastReportTime;
	steps_t allSteps;
	steps_t stepsSoFar = 0;
	steps_t substepsSoFar, substepsToSkip;
	completion_t completion;
	bool visible;
	std::vector<const char*> subtaskNames;
	bool reported = false;
	
	steps_t calcStepsToSkip(const double secondsToSkip, const double secondsPerStep) const;
	bool checkReportInterval(const bool force);
	static void printTime(const double seconds);
	void clearReport(const bool clearStage);
	void reportStage() const;
	void reportProgress();
	void reportProgress(const calcSubstepCompletion_t &calcSubstepCompletion) { completion = calcSubstepCompletion() / allSteps + calcStepCompletion(); return reportProgress(); }
	void handleStep(const calcSubstepCompletion_t &calcSubstepCompletion, const bool force) { if (checkReportInterval(force)) reportProgress(calcSubstepCompletion); }
	
	completion_t calcStepCompletion() const { return static_cast<completion_t>(stepsSoFar - 1) / allSteps; }
	
public:
	class CerrGuard
	{
		Progress &progress;
		const bool reportedBefore;
		CerrGuard(Progress &progress) : progress(progress), reportedBefore(progress.reported) { progress.clearReport(true); std::clog << std::flush; }
		friend class Progress;
	public:
		CerrGuard(const CerrGuard&) = delete;
		CerrGuard& operator=(const CerrGuard&) = delete;
		~CerrGuard() { if (reportedBefore) progress.reportProgress(); }
		template<typename T>
		const CerrGuard& operator<<(T val) const { std::cerr << std::forward<T>(val); return *this; }
	};
	
	class SubtaskGuard
	{
		Progress &progress;
		SubtaskGuard(Progress &progress) : progress(progress) {}
		friend class Progress;
	public:
		SubtaskGuard(const SubtaskGuard&) = delete;
		SubtaskGuard& operator=(const SubtaskGuard&) = delete;
		~SubtaskGuard() { progress.subtaskNames.pop_back(); }
	};
	
	template<typename T = std::size_t>
	class CountingSubsteps
	{
		Progress &progress;
		T i = 0;
		const calcSubstepCompletion_t calcCompletion;
		CountingSubsteps(Progress &progress, const completion_t n) : progress(progress), calcCompletion([&i = std::as_const(i), n](){ return static_cast<completion_t>(i) / n; }) {}
		friend class Progress;
	public:
		CountingSubsteps(const CountingSubsteps&) = delete;
		CountingSubsteps& operator=(const CountingSubsteps&) = delete;
		void substep(const bool force = false) { progress.substep(calcCompletion, force); ++i; }
		void substep(const T increment, const bool force = false) { progress.substep(calcCompletion, force); i += increment; }
	};
	
	Progress(const Stage stage, const char processName[], const steps_t allSteps, const bool visible = true);
	Progress(const Progress&) = delete;
	Progress& operator=(const Progress&) = delete;
	~Progress() { clearReport(true); }
	
	[[nodiscard]] bool isVisible() const { return visible; }
	[[nodiscard]] CerrGuard cerr() { return {*this}; }
	
	void step(const bool force = false);
	void substep(const calcSubstepCompletion_t &calcSubstepCompletion, const bool force = false) { if (visible) { if (--substepsToSkip == 0 || force) [[unlikely]] handleStep(calcSubstepCompletion, force); ++substepsSoFar; } }
	template<typename T = std::size_t>
	[[nodiscard]] CountingSubsteps<T> makeCountingSubsteps(const completion_t n) { return {*this, n}; }
	
	[[nodiscard]] SubtaskGuard enterSubtask(const char subtaskName[]) { subtaskNames.push_back(subtaskName); return {*this}; }
};
