#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#include "event.h"
#include <typeinfo>
#include "refptr.h"
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "gideros_p.h"

//#define GID_CHECK_TYPES

class EventDispatcher;

class SlotBase
{
public:
	virtual ~SlotBase() {}
	virtual void call(Event* event) = 0;
#ifdef GID_CHECK_TYPES
	virtual const std::type_info& t1() const = 0;
	virtual const std::type_info& t2() const = 0;
#endif
	virtual EventDispatcher* object() const = 0;
};

template <class T1, class T2>
class Slot : public SlotBase
{
public:
	Slot(T1* obj, void (T1::*slot)(T2*)) : obj_(obj), slot_(slot)
	{
		//Event* e = (T2 *)0;
	}

	virtual void call(Event* event)
	{
		(obj_->*slot_)(static_cast<T2*>(event));
	}

	virtual EventDispatcher* object() const
	{
		return obj_;
	}

	bool operator == (const Slot& rhs)
	{
		return	obj_ == rhs.obj_ &&
				slot_ == rhs.slot_;
	}

	bool equals(const SlotBase* base)
	{
#ifdef GID_CHECK_TYPES
		if (t1() == base->t1() && t2() == base->t2())
#endif
			return *this == *static_cast<const Slot*>(base);
#ifdef GID_CHECK_TYPES
		return false;
#endif
	}

#ifdef GID_CHECK_TYPES
	const std::type_info& t1() const
	{
		return typeid(T1);
	}

	const std::type_info& t2() const
	{
		return typeid(T2);
	}
#endif
private:
	T1* obj_;
	void (T1::*slot_)(T2*);
};

class GIDEROS_API EventDispatcher : public GReferenced
{
public:
	EventDispatcher()
	{
        slots_=nullptr;
        sources_=nullptr;
        targets_=nullptr;
		allEventDispatchers_.insert(this);
	}
	
	virtual ~EventDispatcher()
	{
        if (sources_) {
            std::vector<EventDispatcher*> sources(sources_->begin(), sources_->end());
            for (std::size_t i = 0; i < sources.size(); ++i)
                sources[i]->removeEventListeners(this);
            delete sources_;
            sources_=nullptr;
        }

        removeEventListeners();
        if (targets_) {
            for (std::set<EventDispatcher*>::iterator iter = targets_->begin(); iter != targets_->end(); ++iter)
                (*iter)->removeSource(this);

            delete targets_;
            targets_=nullptr;
        }

        if (slots_)
            delete slots_;

        allEventDispatchers_.erase(this);
	}

	template <class T0, class T1, class T2>
	void addEventListener(const EventType<T0>& type, T1* obj, void (T1::*func)(T2*))
	{
		T2* typecheck = (T0*)0;

		Slot<T1, T2> slot(obj, func);

        if (slots_==nullptr) slots_=new map_t;

        std::vector<SlotBase*>& slots = (*slots_)[type.id()];

		bool isfound = false;
		for (std::size_t i = 0; i < slots.size(); ++i)
            if (slots[i] && slot.equals(slots[i]))
			{
				isfound = true;
				break;
			}

		if (isfound == false)
		{
			slots.push_back(new Slot<T1, T2>(slot));
			obj->addSource(this);
			addTarget(obj);
			eventListenersChanged();
		}
	}

	template <class T0, class T1, class T2>
	void addEventListener(const EventType<T0>& type, void (T1::*func)(T2*))
	{
		addEventListener(type, static_cast<T1*>(this), func);
	}

	template <class T0, class T1, class T2>
	void removeEventListener(const EventType<T0>& type, T1* obj, void (T1::*func)(T2*))
	{
		T2* typecheck = (T0*)0;

        if (slots_==nullptr) return;
		Slot<T1, T2> slot(obj, func);

        std::vector<SlotBase*>& slots = (*slots_)[type.id()];

		for (std::size_t i = 0; i < slots.size(); ++i)
            if (slots[i] && slot.equals(slots[i]))
			{
				delete slots[i];
                slots[i] = NULL;
				eventListenersChanged();
				break;
			}
	}

	template <class T0, class T1, class T2>
	void removeEventListener(const EventType<T0>& type, void (T1::*func)(T2*))
	{
		removeEventListener(type, static_cast<T1*>(this), func);
	}

	void removeEventListeners(EventDispatcher* obj)
	{
        if (slots_==nullptr) return;

        for (map_t::iterator iter = slots_->begin(); iter != slots_->end(); ++iter)
		{
			std::vector<SlotBase*>& slots = iter->second;
            for (std::size_t i = 0; i < slots.size(); ++i)
            {
                if (slots[i] && slots[i]->object() == obj)
				{
					delete slots[i];
                    slots[i] = NULL;
				}
			}
		}

		obj->removeSource(this);
		removeTarget(obj);
		eventListenersChanged();
	}

	void removeEventListeners()
	{
        if (slots_==nullptr) return;
        for (map_t::iterator iter = slots_->begin(); iter != slots_->end(); ++iter)
		{
			std::vector<SlotBase*>& slots = iter->second;
			for (std::size_t i = 0; i < slots.size(); ++i)
                if (slots[i])
                {
                    delete slots[i];
                    slots[i] = NULL;
                }
		}

        eventListenersChanged();
	}

	void dispatchEvent(Event* event)
	{
        if (slots_==nullptr) return;
        event->setTarget(this);

        map_t::iterator iter = slots_->find(event->id());

        if (iter != slots_->end())
		{
            std::vector<SlotBase*>& slots = iter->second;

            size_t n = slots.size();
            for (std::size_t i = 0; i < n; ++i)
                if (slots[i])
                    slots[i]->call(event);

            slots.erase(std::remove(slots.begin(), slots.end(), (SlotBase*)0), slots.end());
        }
	}

	static void broadcastEvent(Event* event);

	bool hasEventListener() const
	{
        if (slots_==nullptr) return false;
        for (map_t::const_iterator iter = slots_->begin(); iter != slots_->end(); ++iter)
        {
            const std::vector<SlotBase*>& slots = iter->second;
            for (std::size_t i = 0; i < slots.size(); ++i)
                if (slots[i])
                    return true;
        }

        return false;
	}

	template <class T0>
	bool hasEventListener(const EventType<T0>& type) const
	{
        if (slots_==nullptr) return false;
        map_t::const_iterator iter = slots_->find(type.id());

        if (iter == slots_->end())
            return false;

        const std::vector<SlotBase*>& slots = iter->second;
        for (std::size_t i = 0; i < slots.size(); ++i)
            if (slots[i])
                return true;

        return false;
    }

protected:
	virtual void eventListenersChanged() {}

private:
	void addSource(EventDispatcher* ptr)
	{
        if (sources_==nullptr)
            sources_=new std::set<EventDispatcher*>();
        sources_->insert(ptr);
	}

	void removeSource(EventDispatcher* ptr)
	{
        if (sources_==nullptr) return;
        sources_->erase(ptr);
	}

	void addTarget(EventDispatcher* ptr)
	{
        if (targets_==nullptr)
            targets_=new std::set<EventDispatcher*>();
        targets_->insert(ptr);
	}

	void removeTarget(EventDispatcher* ptr)
	{
        if (targets_==nullptr) return;
        targets_->erase(ptr);
	}

	typedef std::map<int, std::vector<SlotBase*> > map_t;
    map_t *slots_;
    std::set<EventDispatcher*> *sources_;
    std::set<EventDispatcher*> *targets_;

protected:
	static std::set<EventDispatcher*> allEventDispatchers_;
};

#endif
