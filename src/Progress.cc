#include "./Progress.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <ios>
#include <stdexcept>

#include "options.hh"


Progress::calcStepCompletion_t Progress::calc0StepCompletion = [](){ return 0.0; };

std::uint_fast8_t Progress::stageCounters[STAGE_COUNT] = {};

Progress *Progress::progress = nullptr;
Progress::timePoint_t Progress::programStartTime;

Progress::steps_t Progress::calcStepsToSkip(const double secondsToSkip, const double secondsPerStep) const
{
	const steps_t newSubstepsToSkip = static_cast<steps_t>(secondsToSkip / secondsPerStep);
	return std::max<steps_t>(1, newSubstepsToSkip);
}

double Progress::getSecondsSinceStart(const timePoint_t currentTime)
{
	return std::chrono::duration<double>(currentTime - programStartTime).count();
}

bool Progress::checkProgramRunTime(const timePoint_t currentTime)
{
	static bool alreadyPassed = false;
	if (alreadyPassed) [[likely]]
		return true;
	const double secondsSinceStart = getSecondsSinceStart(currentTime);
	alreadyPassed = secondsSinceStart >= reportInterval;
	return alreadyPassed;
}

bool Progress::checkReportInterval(const bool force)
{
	if (substepsSoFar == 0)
	{
		substepsToSkip = 1;
		return force && checkProgramRunTime(std::chrono::steady_clock::now());
	}
	const timePoint_t currentTime = std::chrono::steady_clock::now();
	const double secondsSinceStart = std::chrono::duration<double>(currentTime - startTime).count();
	const double secondsSinceLastReport = std::chrono::duration<double>(currentTime - lastReportTime).count();
	const double remainingSeconds = reportInterval - secondsSinceLastReport;
	const double secondsPerStep = secondsSinceStart / substepsSoFar;
	if (!checkProgramRunTime(currentTime) || (!force && remainingSeconds > secondsPerStep)) // Not even `force` can show progress before the initial delay has passed. This is by design.
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
	static constexpr std::uintmax_t tooManySeconds = 10 * 365 * 24 * 60 * 60;  // 10 years
	if (time >= tooManySeconds)
	{
		std::clog << "never (more or less)";
		return;
	}
	
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
	if (reportLines != 0)
	{
		std::clog << "\033[" << (clearStage ? reportLines : reportLines - 1) << "A\r\033[J";
		reportLines = clearStage ? 0 : 1;
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

void Progress::reportSingleLevel(const bool indentMore, const completion_t completion, const double et, const double eta)
{
	std::clog << "    ";
	if (indentMore)
		std::clog << "    ";
	const std::uint_fast8_t barLength = indentMore ? 70 : 74;
	const std::uint_fast8_t progressBarLenght = static_cast<std::uint_fast8_t>(barLength * completion);
	std::clog << '[' << std::setfill('#') << std::setw(progressBarLenght) << "" << std::setfill('.') << std::setw(barLength - progressBarLenght) << "" << ']' << '\n';
	
	std::ios oldClogState(nullptr);
	oldClogState.copyfmt(std::clog);
	std::clog << "    ";
	if (indentMore)
		std::clog << "    ";
	std::clog << std::fixed << "Estimated completion: " << std::setprecision(5) << completion * 100.0 << "%  ET: ";
	printTime(et);
	std::clog << "  ETA: ";
	if (std::isnan(eta))
		std::clog << "N/A";
	else
		printTime(eta);
	std::clog << std::endl;
	std::clog.copyfmt(oldClogState);
}

void Progress::reportProgress()
{
	if (reportLines == 0)
		reportStage();
	
	clearReport(false);
	
	const timePoint_t currentTime = std::chrono::steady_clock::now();
	
	const completion_t completion = (stepCompletion + stepsSoFar - 1) / allSteps;
	const double finishedStepsSeconds = std::chrono::duration<double>(stepStartTime - startTime).count();
	const double currentStepSeconds = std::chrono::duration<double>(currentTime - stepStartTime).count();
	const double estimatedStepTime = (currentStepSeconds < 0.1 || stepCompletion == 0.0) ? NAN : currentStepSeconds * (1.0 / stepCompletion - 1.0);
	const double estimatedTime = std::isnan(estimatedStepTime)
		? ((finishedStepsSeconds < 0.1 || stepsSoFar == 1) ? NAN : finishedStepsSeconds * (static_cast<double>(allSteps) / static_cast<double>(stepsSoFar - 1) - 1.0))
		: estimatedStepTime + (finishedStepsSeconds + estimatedStepTime / (1.0 - stepCompletion)) * (static_cast<double>(allSteps) / static_cast<double>(stepsSoFar) - 1.0);
	
	const bool twoBars = allSteps != 1 && !std::isnan(estimatedStepTime);
	
	std::clog << "    " << processName;
	if (allSteps == 1)
		for (const auto &subtaskName : subtaskNames)
			std::clog << " -> " << subtaskName;
	std::clog << "...\n";
	reportSingleLevel(false, completion, finishedStepsSeconds + currentStepSeconds, estimatedTime);
	
	if (allSteps != 1)
	{
		std::clog << "        Step " << stepsSoFar << '/' << allSteps;
		for (const auto &subtaskName : subtaskNames)
			std::clog << " -> " << subtaskName;
		std::clog << "...\n";
		if (twoBars)
			reportSingleLevel(true, stepCompletion, currentStepSeconds, estimatedStepTime);
	}
	
	reportLines = twoBars ? 7 : (allSteps == 1 ? 4 : 5);
}

void Progress::handleStep(const calcStepCompletion_t &calcStepCompletion, const bool force)
{
	if (checkReportInterval(force))
		reportProgress(calcStepCompletion);
}

void Progress::init()
{
	programStartTime = std::chrono::steady_clock::now();
}

Progress::Progress(const Stage stage, const char processName[], const steps_t allSteps, const bool visible) :
	stage(stage),
	processName(processName),
	allSteps(allSteps),
	visible(visible && options::status.getValue())
{
	if (progress != nullptr) [[unlikely]]
		throw std::runtime_error("There cannot be two object of class \"Progress\" existing at the same time!");
	else
		progress = this;
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
		stepStartTime = std::chrono::steady_clock::now();
		handleStep(calc0StepCompletion, force);
		lastReportTime = stepStartTime;
		substepsSoFar = 0;
		substepsToSkip = 2;
	}
}

void Progress::reportTime(const std::string_view eventName)
{
	CerrGuard cerrGuard = cerr();
	std::clog << "Time at \"" << eventName << "\": ";
	printTime(getSecondsSinceStart(std::chrono::steady_clock::now()));
	std::clog << std::endl;
}
