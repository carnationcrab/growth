#include "util/PerformanceLogger.hpp"
#include "Types.hpp"
#include "base/gateway/Calgorithm.hpp"
#include "base/gateway/Ciomanip.hpp"
#include "base/gateway/Csstream.hpp"

namespace growth {
namespace perf {

PerformanceLogger::Scope::Scope(PerformanceLogger &logger, const char *label)
    : logger_(&logger), start_(Cchrono::now()) {
	index_ = logger.samples_.size();
	logger.samples_.push_back(Sample{label ? label : "", 0.0});
}

PerformanceLogger::Scope::Scope(Scope &&other) noexcept
    : logger_(other.logger_), index_(other.index_), start_(other.start_) {
	other.logger_ = nullptr;
}

PerformanceLogger::Scope &PerformanceLogger::Scope::operator=(Scope &&other) noexcept {
	if (this != &other) {
		if (logger_)
			logger_->finish_scope(index_, Cchrono::elapsed_millis(start_, Cchrono::now()));
		logger_ = other.logger_;
		index_  = other.index_;
		start_  = other.start_;
		other.logger_ = nullptr;
	}
	return *this;
}

PerformanceLogger::Scope::~Scope() {
	if (logger_)
		logger_->finish_scope(index_, Cchrono::elapsed_millis(start_, Cchrono::now()));
}

PerformanceLogger::Scope PerformanceLogger::scope(const char *label) {
	return Scope(*this, label);
}

void PerformanceLogger::clear() {
	samples_.clear();
}

void PerformanceLogger::finish_scope(Size index, double milliseconds) {
	if (index < samples_.size())
		samples_[index].milliseconds = milliseconds;
}

Vector<String> PerformanceLogger::format_report() const {
	Vector<String> lines;
	if (samples_.empty())
		return lines;

	double total_ms = 0.0;
	for (const Sample &s : samples_)
		total_ms += s.milliseconds;

	Vector<Sample> sorted = samples_;
	Calgorithm::sort(sorted.begin(), sorted.end(), [](const Sample &a, const Sample &b) {
		return a.milliseconds > b.milliseconds;
	});

	OStringStream os;
	os << Ciomanip::fixed() << Ciomanip::setprecision(2);
	for (const Sample &s : sorted) {
		const double pct = total_ms > 0.0 ? (100.0 * s.milliseconds / total_ms) : 0.0;
		os.str("");
		os.clear();
		os << "[Perf] " << s.label << ": " << s.milliseconds << " ms (" << pct << "%)";
		lines.push_back(os.str());
	}
	os.str("");
	os.clear();
	os << "[Perf] total: " << total_ms << " ms";
	lines.push_back(os.str());
	return lines;
}

} // namespace perf
} // namespace growth
