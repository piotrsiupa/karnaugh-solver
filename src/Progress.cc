#include "./Progress.hh"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <ios>

#include "options.hh"


Progress::calcSubstepCompletion_t Progress::calc0SubstepCompletion = [](){ return 0.0; };

std::uint_fast8_t Progress::stageCounters[STAGE_COUNT] = {};

Progress::steps_t Progress::calcStepsToSkip(const double secondsToSkip, const double secondsPerStep) const
{
	const steps_t newSubstepsToSkip = static_cast<steps_t>(secondsToSkip / secondsPerStep);
	return std::max<steps_t>(1, newSubstepsToSkip);
}

bool Progress::checkReportInterval(const bool force)
{
	if (substepsSoFar == 0) [[unlikely]]
	{
		substepsToSkip = 1;
		return false;
	}
	const timePoint_t currentTime = std::chrono::steady_clock::now();
	const double secondsSinceStart = std::chrono::duration<double>(currentTime - startTime).count();
	const double secondsSinceLastReport = std::chrono::duration<double>(currentTime - lastReportTime).count();
	const double remainingSeconds = reportInterval - secondsSinceLastReport;
	const double secondsPerStep = secondsSinceStart / substepsSoFar;
	if (secondsSinceStart < 1.0 || (!force && remainingSeconds > secondsPerStep)) // Not even `force` can show progress before 1 second has passed. This is by design.
	{
		substepsToSkip = calcStepsToSkip(remainingSeconds, secondsPerStep);
		return false;
	}
	else
	{
		lastReportTime = currentTime;
		substepsToSkip = calcStepsToSkip(reportInterval, secondsPerStep);
		return true;
	}
}

void Progress::printTime(double time)
{
	std::uintmax_t x = static_cast<std::uintmax_t>(time);
	const std::uint_fast8_t seconds = static_cast<std::uint_fast8_t>(x % 60);
	x /= 60;
	time -= x * 60 + seconds / 10 * 10;
	const std::uint_fast8_t minutes = static_cast<std::uint_fast8_t>(x % 60);
	x /= 60;
	const uint_fast8_t hours = static_cast<std::uint_fast8_t>(x % 24);
	x /= 24;
	const uint_fast8_t days = static_cast<std::uint_fast8_t>(x % 365);
	x /= 365;
	const uintmax_t years = x;
	if (years != 0)
	{
		std::clog << static_cast<unsigned>(years) << " year";
		if (years > 1)
			std::clog << 's';
		std::clog << ' ';
	}
	if (days != 0)
	{
		std::clog << static_cast<unsigned>(days) << " day";
		if (days > 1)
			std::clog << 's';
		std::clog << ' ';
	}
	std::clog << std::setfill('0')
			<< std::setw(2) << static_cast<unsigned>(hours) << ':'
			<< std::setw(2) << static_cast<unsigned>(minutes) << ':'
			<< static_cast<unsigned>(seconds / 10) << std::setprecision(3) << time;
}

void Progress::clearReport(const bool clearStage)
{
	if (reported)
	{
		if (clearStage)
			std::clog << "\033[4A";
		else
			std::clog << "\033[3A";
		std::clog << "\r\033[J";
		reported = false;
	}
}

void Progress::reportStage() const
{
	std::clog << "Stage " << (static_cast<unsigned>(stage) + 1) << '/' << STAGE_COUNT;
	std::clog << " - ";
	switch (stage)
	{
	case Stage::LOADING:
		std::clog << "Loading";
		break;
	case Stage::SOLVING:
		std::clog << "Solving";
		break;
	case Stage::OPTIMIZING:
		std::clog << "Optimizing";
		break;
	}
	std::clog << " (sub-stage " << static_cast<unsigned>(stageCounters[static_cast<std::size_t>(stage)]) << ')';
	std::clog << '\n';
}

void Progress::reportProgress()
{
	if (!reported) [[unlikely]]
		reportStage();
	
	clearReport(false);
	
	std::clog << processName;
	if (allSteps != 1)
		std::clog << ' ' << '(' << stepsSoFar << '/' << allSteps << ')';
	for (const auto &subtaskName : subtaskNames)
		std::clog << " -> " << subtaskName;
	std::clog << "...\n";
	
	const std::uint_fast8_t progressBarLenght = static_cast<std::uint_fast8_t>(78 * completion);
	std::clog << '[' << std::setfill('#') << std::setw(progressBarLenght) << "" << std::setfill('.') << std::setw(78 - progressBarLenght) << "" << ']' << '\n';
	
	const timePoint_t currentTime = std::chrono::steady_clock::now();
	const double secondsSinceStart = std::chrono::duration<double>(currentTime - startTime).count();
	std::ios oldClogState(nullptr);
	oldClogState.copyfmt(std::clog);
	std::clog << std::fixed << "Estimated completion: " << std::setprecision(5) << completion * 100.0 << "%  ET: ";
	printTime(secondsSinceStart);
	std::clog << "  ETA: ";
	if (secondsSinceStart < 0.1 || completion == 0.0)
	{
		std::clog << "N/A";
	}
	else
	{
		const double estimatedTime = secondsSinceStart / completion * (1.0 - completion);
		printTime(estimatedTime);
	}
	std::clog << std::endl;
	std::clog.copyfmt(oldClogState);
	
	reported = true;
}

Progress::Progress(const Stage stage, const char processName[], const steps_t allSteps, const bool visible) :
	stage(stage),
	processName(processName),
	allSteps(allSteps),
	visible(visible && options::status.getValue())
{
	assert(static_cast<std::size_t>(stage) < STAGE_COUNT);
	if (static_cast<std::size_t>(stage) != STAGE_COUNT - 1)
		assert(stageCounters[static_cast<std::size_t>(stage) + 1] == 0);
	++stageCounters[static_cast<std::size_t>(stage)];
	if (visible)
		lastReportTime = startTime = std::chrono::steady_clock::now();
}

void Progress::step(const bool force)
{
	if (visible)
	{
		++stepsSoFar;
		handleStep(calc0SubstepCompletion, force);
		lastReportTime = std::chrono::steady_clock::now();
		substepsSoFar = 0;
		substepsToSkip = 2;
	}
}
