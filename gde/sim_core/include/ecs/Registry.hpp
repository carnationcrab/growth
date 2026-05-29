#pragma once

#include "ComponentStorage.hpp"
#include "EntityId.hpp"
#include "IComponentStorage.hpp"
#include "base/gateway/Cassert.hpp"
#include "base/gateway/Cfunctional.hpp"
#include "base/gateway/Cmemory.hpp"
#include "base/gateway/Ctypeindex.hpp"
#include "base/gateway/Cunordered_map.hpp"
#include "base/gateway/Cvector.hpp"

namespace growth {

/// Single public ECS API: create/destroy entities, add/remove/get components, iterate views.
/// Storage is encapsulated (sparse set + SoA); swap by replacing the backend later if needed.
class Registry {
public:
	Registry() = default;
	Registry(Registry &&) = default;
	Registry &operator=(Registry &&) = default;
	Registry(const Registry &) = delete;
	Registry &operator=(const Registry &) = delete;

	EntityId create() {
		EntityId e;
		if (!free_list_.empty()) {
			e = free_list_.back();
			free_list_.pop_back();
		} else {
			if (next_id_ == ENTITY_NULL) ++next_id_;
			e = next_id_++;
		}
		return e;
	}

	void destroy(EntityId e) {
		if (e == ENTITY_NULL) return;
		for (auto &p : storages_)
			p->remove(e);
		free_list_.push_back(e);
	}

	template<typename T>
	bool has(EntityId e) const {
		IComponentStorage *s = find_storage<T>();
		return s && s->has(e);
	}

	/// Requires has<T>(e). Use try_get for nullable access.
	template<typename T>
	T &get(EntityId e) {
		ComponentStorage<T> *s = find_storage<T>();
		assert(s && "component type not in use");
		T *p = s->get(e);
		assert(p && "entity does not have this component");
		return *p;
	}

	template<typename T>
	const T &get(EntityId e) const {
		const ComponentStorage<T> *s = find_storage<T>();
		assert(s && "component type not in use");
		const T *p = s->get(e);
		assert(p && "entity does not have this component");
		return *p;
	}

	template<typename T>
	T *try_get(EntityId e) {
		ComponentStorage<T> *s = find_storage<T>();
		return s ? s->get(e) : nullptr;
	}

	template<typename T>
	const T *try_get(EntityId e) const {
		const ComponentStorage<T> *s = find_storage<T>();
		return s ? s->get(e) : nullptr;
	}

	template<typename T, typename... Args>
	T &emplace(EntityId e, Args &&... args) {
		return ensure_storage<T>()->emplace(e, Cutility::forward<Args>(args)...);
	}

	template<typename T>
	void remove(EntityId e) {
		IComponentStorage *s = find_storage<T>();
		if (s) s->remove(e);
	}

	/// Iterate entities that have all of A, B, C; call f(EntityId e, A& a, B& b, C& c).
	/// Uses shared view_cache_ (packed list); one buffer reused for all views, no per-entity alloc.
	template<typename A, typename B, typename C, typename Func>
	void each(Func &&f) {
		ComponentStorage<A> *sa = find_storage<A>();
		ComponentStorage<B> *sb = find_storage<B>();
		ComponentStorage<C> *sc = find_storage<C>();
		if (!sa || !sb || !sc) return;
		IComponentStorage *smallest = sa;
		if (sb->dense_count() < smallest->dense_count()) smallest = sb;
		if (sc->dense_count() < smallest->dense_count()) smallest = sc;
		view_cache_.clear();
		view_cache_.reserve(smallest->dense_count());
		for (size_t i = 0; i < smallest->dense_count(); ++i) {
			EntityId e = smallest->dense_entity(i);
			if (sa->has(e) && sb->has(e) && sc->has(e))
				view_cache_.push_back(e);
		}
		for (EntityId e : view_cache_)
			f(e, *sa->get(e), *sb->get(e), *sc->get(e));
	}

	/// Two-component view. Uses shared view_cache_ (packed list).
	template<typename A, typename B, typename Func>
	void each(Func &&f) {
		ComponentStorage<A> *sa = find_storage<A>();
		ComponentStorage<B> *sb = find_storage<B>();
		if (!sa || !sb) return;
		IComponentStorage *smallest = (sa->dense_count() <= sb->dense_count()) ? sa : sb;
		view_cache_.clear();
		view_cache_.reserve(smallest->dense_count());
		for (size_t i = 0; i < smallest->dense_count(); ++i) {
			EntityId e = smallest->dense_entity(i);
			if (sa->has(e) && sb->has(e))
				view_cache_.push_back(e);
		}
		for (EntityId e : view_cache_)
			f(e, *sa->get(e), *sb->get(e));
	}

	/// Single-component iteration.
	template<typename A, typename Func>
	void each(Func &&f) {
		ComponentStorage<A> *sa = find_storage<A>();
		if (!sa) return;
		for (size_t i = 0; i < sa->dense_count(); ++i) {
			EntityId e = sa->dense_entity(i);
			f(e, *sa->get(e));
		}
	}

private:
	template<typename T>
	ComponentStorage<T> *ensure_storage() {
		TypeIndex key = TypeIndex(typeid(T));
		auto it = storages_map_.find(key);
		if (it != storages_map_.end())
			return static_cast<ComponentStorage<T> *>(it->second);
		auto s = Cmemory::make_unique<ComponentStorage<T>>();
		ComponentStorage<T> *p = s.get();
		storages_.push_back(Cutility::move(s));
		storages_map_[key] = p;
		return p;
	}

	template<typename T>
	ComponentStorage<T> *find_storage() {
		auto it = storages_map_.find(TypeIndex(typeid(T)));
		return it != storages_map_.end() ? static_cast<ComponentStorage<T> *>(it->second) : nullptr;
	}

	template<typename T>
	const ComponentStorage<T> *find_storage() const {
		auto it = storages_map_.find(TypeIndex(typeid(T)));
		return it != storages_map_.end() ? static_cast<const ComponentStorage<T> *>(it->second) : nullptr;
	}

	Vector<UniquePtr<IComponentStorage>> storages_;
	UnorderedMap<TypeIndex, IComponentStorage *> storages_map_;
	Vector<EntityId> free_list_;
	EntityId next_id_ = 1;

	/// Reused for each<A,B> and each<A,B,C> (Option 2: packed iteration). Cleared and refilled per view.
	Vector<EntityId> view_cache_;
};

} // namespace growth
