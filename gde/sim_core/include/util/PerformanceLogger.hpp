#pragma once

#include "Types.hpp"
#include "base/gateway/Cchrono.hpp"
#include "base/gateway/Cstring.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {
namespace perf {

struct Sample {
	String label;
	double milliseconds = 0.0;
};

/// Scoped wall-clock timings for profiling (Godot-free; usable from bench and pipeline).
class PerformanceLogger {
public:
	class Scope {
	public:
		Scope(PerformanceLogger &logger, const char *label);
		Scope(const Scope &)            = delete;
		Scope &operator=(const Scope &) = delete;
		Scope(Scope &&other) noexcept;
		Scope &operator=(Scope &&other) noexcept;
		~Scope();

	private:
		PerformanceLogger *logger_ = nullptr;
		Size index_              = 0;
		TimePoint start_;
	};

	Scope scope(const char *label);

	void clear();
	const Vector<Sample> &samples() const { return samples_; }
	Vector<String> format_report() const;

private:
	friend class Scope;

	void finish_scope(Size index, double milliseconds);

	Vector<Sample> samples_;
};

} // namespace perf
} // namespace growth
