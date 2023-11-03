#include "./Progress.hh"

#include <algorithm>
#include <iomanip>
#include <ios>
#include <iostream>


Progress::calcSubstepCompletion_t Progress::calc0SubstepCompletion = [](){ return 0.0; };

Progress::steps_t Progress::calcStepsToSkip(const double secondsToSkip, const double secondsPerStep) const
{
	const steps_t newSubstepsToSkip = static_cast<steps_t>(secondsToSkip / secondsPerStep);
	return std::max<steps_t>(1, newSubstepsToSkip);
}

bool Progress::checkReportInterval(const bool force)
{
	if (substepsSoFar == 0)
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

void Progress::clearReport()
{
	if (reportVisible)
	{
		std::clog << "\033[3A\r\033[J";
		reportVisible = false;
	}
}

void Progress::reportProgress(const calcSubstepCompletion_t &calcSubstepCompletion)
{
	const completion_t completion = calcSubstepCompletion() / allSteps + calcStepCompletion();
	
	clearReport();
	
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
	
	reportVisible = true;
}

void Progress::handleStep(const calcSubstepCompletion_t &calcSubstepCompletion, const bool force)
{
	if (checkReportInterval(force))
		reportProgress(calcSubstepCompletion);
}

Progress::Progress(const char processName[], const steps_t allSteps, const bool progressVisible) :
	processName(processName),
	allSteps(allSteps),
	progressVisible(progressVisible)
{
	if (progressVisible)
	{
		lastReportTime = startTime = std::chrono::steady_clock::now();
		substepsToSkip = 2;
	}
}
