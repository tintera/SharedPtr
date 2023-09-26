#pragma once
#include <atomic>

/// Lock free smart ptr similar to shared ptr.
///	- Destructor of pointed object must not throw. Or operators =, == have undefined behavior.
///	- Published as single header file for readability.
///
/// Known limits:
///	- Some race condition exist. Best to fix them and keep implementation lock free. And keep default constructor noexcept (as in std::)
///	- Owned object is not part of control block.
/// - No custom deleter or allocator.
///	- No separate template type for constructors. (std::shared_ptr constructor has another template type Y)
///	- No make_shared function.
///	- No std::hash<std::shared_ptr>
///	- No std::atomic<std::shared_ptr>
///
/// Omitted (not much to learn in implementing them IMHO)
/// - reset
///	- swap
///	- operator[] managing arrays not implemented at all.
///	- unique as it's removed in C++ 20
///	- owner_before
/// - operator<<(std::shared_ptr)
///
namespace smart_ptr
{

template<typename T>
class weak_ptr;

template<typename T>
class shared_ptr
{
	friend class weak_ptr<T>;

	struct control_block
	{
		explicit control_block(T* payload)
			: payload_(payload)
		{
		}

		std::atomic<int> usages_{1};
		std::atomic<int> weak_usages_{0};
		T* payload_{nullptr};
	};

	control_block* control_{nullptr};

	void finish_one_instance_()
	{
		if (!control_)
		{
			return;
		}
		--control_->usages_;
		if (control_->usages_ == 0)
		{
			// Last owner. We are in a single threaded scenario now.
			delete control_->payload_;
			if (control_->weak_usages_ == 0)
			{
				delete control_;
			}
		}
	}

public:
	constexpr shared_ptr() noexcept = default;

	constexpr explicit shared_ptr(nullptr_t) noexcept{}

	explicit shared_ptr(T* ptr)
	try
		: control_(new control_block(ptr))
	{
	}
	catch(...)
	{
		delete ptr;
		throw;
	}

	explicit shared_ptr(std::unique_ptr<T>&& ptr)
		: control_(new control_block(ptr.get()))
	{
	}

	~shared_ptr() noexcept
	{
		finish_one_instance_();
	}

	shared_ptr(const shared_ptr& other) noexcept
		: control_{other.control_}
	{
		if(control_)
		{
			// TODO: Race condition, control can be deleted before usages_ is incremented.
			++control_->usages_;
		}
	}

	shared_ptr(shared_ptr&& other) noexcept
	{
		std::swap(control_, other.control_);
	}

	template< class Y >
	explicit shared_ptr( const weak_ptr<Y>& r )
	{
		if (r.expired())
		{
			throw std::bad_weak_ptr{};
		}
		control_ = r.control_;
		++control_->usages_;
	}

	shared_ptr& operator=(const shared_ptr& other) noexcept
	{
		if(this == &other) {
			return *this;
		}
		finish_one_instance_();
		control_ = other.control_;
		++other.use_count();
		return *this;
	}

	shared_ptr& operator=(shared_ptr&& other) noexcept
	{
		std::swap(this, shared_ptr{other}); 
		return *this;
	}

	[[nodiscard]] explicit operator bool() const noexcept
	{
		return static_cast<bool>(control_);
	}
	
	void reset() noexcept 
	{
		finish_one_instance_();
		control_->payload_ = nullptr;
	}

	[[nodiscard]] T* get() const noexcept
	{
		// No race condition. The control exists while it's owned by this shared_ptr.
		return control_ ? control_->payload_ : nullptr;
	}

	[[nodiscard]] T& operator*() const noexcept
	{
		return *get();
	}

	[[nodiscard]] T* operator->() const noexcept
	{
		return get();
	}

	[[nodiscard]] long use_count() const noexcept
	{
		// No race condition. The control_ exists while it's owned by this shared_ptr
		return control_ ? control_->usages_.load() : 0;
	}

};

template< class T, class U >
std::strong_ordering operator<=>( const shared_ptr<T>& lhs, const shared_ptr<U>& rhs ) noexcept
{
	return lhs.control_ <=> rhs.control_;
};

template<typename T>
class weak_ptr
{
	friend class shared_ptr<T>;

	typename shared_ptr<T>::control_block* control_{nullptr};
public:
	constexpr weak_ptr() noexcept = default;

	~weak_ptr()
	{
		if (control_)
		{
			--control_->weak_usages_;
			if (control_->usages_ == 0 && control_->weak_usages_ == 0)
			{
				delete control_;
			}
		}
	}

	explicit weak_ptr( const shared_ptr<T>& r ) noexcept
		: control_(r.control_)
	{
		if (control_)
		{
			// TODO: Race condition, control can be deleted before weak_usages_ is incremented.
			++control_->weak_usages_;
		}
	}

	// TODO: explicit dod not compile
	weak_ptr( const weak_ptr& r ) noexcept
	{
		control_ = r.control_;
		if (control_)
		{
			// TODO: Race condition, control can be deleted before weak_usages_ is incremented.
			++r.control_->weak_usages_;
		}
	}

	explicit weak_ptr(weak_ptr&& r) noexcept
	{
		std::swap(*this, r);
	}

	weak_ptr& operator=(const weak_ptr& other) noexcept
	{
		if(this == &other)
		{
			return *this;
		}
		std::swap(*this, weak_ptr{other});
		return *this;
	}

	weak_ptr& operator=(weak_ptr&& r) noexcept
	{
		std::swap(*this, r);
		return *this;
	}

	[[nodiscard]] bool expired() const noexcept
	{
		return (!control_) || (control_->usages_ == 0);
	}

	shared_ptr<T> lock()
	{
		return expired() ? shared_ptr<T>() : shared_ptr<T>(*this);
	}
};

}
