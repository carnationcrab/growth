#pragma once

#include "EntityId.hpp"
#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Ctypeindex.hpp"

namespace growth {

/// Type-erased interface for one component type's storage. Lets Registry and views
/// work without knowing T; also allows swapping to another backend (e.g. archetypes) later.
struct IComponentStorage {
	virtual ~IComponentStorage() = default;

	virtual TypeIndex type_id() const = 0;
	virtual bool has(EntityId e) const = 0;
	virtual void *get_ptr(EntityId e) = 0;
	virtual const void *get_ptr(EntityId e) const = 0;
	virtual void emplace(EntityId e, const void *data) = 0;
	virtual void remove(EntityId e) = 0;

	/// Number of entities that have this component (for view iteration).
	virtual size_t dense_count() const = 0;
	/// Entity at dense index i (0 <= i < dense_count()).
	virtual EntityId dense_entity(size_t i) const = 0;
};

} // namespace growth
