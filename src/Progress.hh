#pragma once

#include <chrono>
#include <functional>
#include <vector>

#include "global.hh"


class Progress
{
public:
	using steps_t = std::uintmax_t;
	using completion_t = double;
	using calcSubstepCompletion_t = std::function<completion_t()>;
	
	static calcSubstepCompletion_t calc0SubstepCompletion;
	
private:
	using timePoint_t = std::chrono::steady_clock::time_point;
	
	static constexpr double reportInterval = 1.0;
	
	const char *const processName;
	timePoint_t startTime, lastReportTime;
	steps_t allSteps;
	steps_t stepsSoFar = 0;
	steps_t substepsSoFar = 0, substepsToSkip;
	std::vector<const char*> subtaskNames;
	bool reportVisible = false;
	
	steps_t calcStepsToSkip(const double secondsToSkip, const double secondsPerStep) const;
	bool checkReportInterval(const bool force);
	static void printTime(const double seconds);
	void clearReport();
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
	
	Progress(const char processName[], const steps_t allSteps);
	Progress(const Progress&) = delete;
	Progress& operator=(const Progress&) = delete;
	~Progress() { clearReport(); }
	
	void step(const bool force = false) { if (::terminalStderr) { ++stepsSoFar; handleStep(calc0SubstepCompletion, force); } }
	void substep(const calcSubstepCompletion_t &calcSubstepCompletion, const bool force = false) { if (::terminalStderr) { if (--substepsToSkip == 0 || force) handleStep(calcSubstepCompletion, force); ++substepsSoFar; } }
	
	SubtaskGuard enterSubtask(const char subtaskName[]) { subtaskNames.push_back(subtaskName); return {*this}; }
};
