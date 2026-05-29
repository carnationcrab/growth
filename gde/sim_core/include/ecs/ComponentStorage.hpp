#pragma once

#include "IComponentStorage.hpp"
#include "base/gateway/Ctypeindex.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Sparse set + SoA storage for one component type. O(1) has/add/remove; cache-friendly iteration.
template<typename T>
class ComponentStorage : public IComponentStorage {
public:
	TypeIndex type_id() const override { return TypeIndex(typeid(T)); }

	bool has(EntityId e) const override {
		if (e >= sparse_.size()) return false;
		size_t i = sparse_[e];
		return i < dense_.size() && dense_[i] == e;
	}

	void *get_ptr(EntityId e) override {
		return has(e) ? &data_[sparse_[e]] : nullptr;
	}
	const void *get_ptr(EntityId e) const override {
		return has(e) ? &data_[sparse_[e]] : nullptr;
	}

	T *get(EntityId e) { return static_cast<T *>(get_ptr(e)); }
	const T *get(EntityId e) const { return static_cast<const T *>(get_ptr(e)); }

	void emplace(EntityId e, const void *data) override {
		if (has(e)) {
			data_[sparse_[e]] = *static_cast<const T *>(data);
			return;
		}
		if (e >= sparse_.size()) sparse_.resize(e + 1, invalid_index());
		size_t i = dense_.size();
		dense_.push_back(e);
		data_.push_back(*static_cast<const T *>(data));
		sparse_[e] = i;
	}

	template<typename... Args>
	T &emplace(EntityId e, Args &&... args) {
		if (has(e)) {
			data_[sparse_[e]] = T(Cutility::forward<Args>(args)...);
			return data_[sparse_[e]];
		}
		if (e >= sparse_.size()) sparse_.resize(e + 1, invalid_index());
		size_t i = dense_.size();
		dense_.push_back(e);
		data_.emplace_back(Cutility::forward<Args>(args)...);
		sparse_[e] = i;
		return data_.back();
	}

	void remove(EntityId e) override {
		if (!has(e)) return;
		size_t i = sparse_[e];
		EntityId last_e = dense_.back();
		dense_[i] = last_e;
		data_[i] = Cutility::move(data_.back());
		sparse_[last_e] = i;
		dense_.pop_back();
		data_.pop_back();
		sparse_[e] = invalid_index();
	}

	size_t dense_count() const override { return dense_.size(); }
	EntityId dense_entity(size_t i) const override { return dense_[i]; }

private:
	static constexpr size_t invalid_index() { return static_cast<size_t>(-1); }

	Vector<EntityId> dense_;
	Vector<size_t> sparse_;
	Vector<T> data_;
};

} // namespace growth
