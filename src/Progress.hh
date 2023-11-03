#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
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
	steps_t substepsSoFar = 0, substepsToSkip;
	bool progressVisible;
	std::vector<const char*> subtaskNames;
	bool reportVisible = false;
	
	steps_t calcStepsToSkip(const double secondsToSkip, const double secondsPerStep) const;
	bool checkReportInterval(const bool force);
	static void printTime(const double seconds);
	void clearReport(const bool clearStage);
	void reportStage() const;
	void reportProgress(const calcSubstepCompletion_t &calcSubstepCompletion);
	void handleStep(const calcSubstepCompletion_t &calcSubstepCompletion, const bool force);
	
	completion_t calcStepCompletion() const { return static_cast<completion_t>(stepsSoFar - 1) / allSteps; }
	
public:
	class SubtaskGuard
	{
		Progress &progress;
		friend class Progress;
		SubtaskGuard(Progress &progress) : progress(progress) {}
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
		friend class Progress;
		CountingSubsteps(Progress &progress, const completion_t n) : progress(progress), calcCompletion([&i = std::as_const(i), n](){ return static_cast<completion_t>(i) / n; }) {}
		CountingSubsteps(const CountingSubsteps&) = delete;
		CountingSubsteps& operator=(const CountingSubsteps&) = delete;
	public:
		void substep(const bool force = false) { progress.substep(calcCompletion, force); ++i; }
		void substep(const T increment, const bool force = false) { progress.substep(calcCompletion, force); i += increment; }
	};
	
	Progress(const Stage stage, const char processName[], const steps_t allSteps, const bool progressVisible = true);
	Progress(const Progress&) = delete;
	Progress& operator=(const Progress&) = delete;
	~Progress() { clearReport(true); }
	
	void step(const bool force = false) { if (progressVisible) { ++stepsSoFar; handleStep(calc0SubstepCompletion, force); } }
	void substep(const calcSubstepCompletion_t &calcSubstepCompletion, const bool force = false) { if (progressVisible) { if (--substepsToSkip == 0 || force) handleStep(calcSubstepCompletion, force); ++substepsSoFar; } }
	template<typename T = std::size_t>
	CountingSubsteps<T> makeCountingSubsteps(const completion_t n) { return {*this, n}; }
	
	SubtaskGuard enterSubtask(const char subtaskName[]) { subtaskNames.push_back(subtaskName); return {*this}; }
};
