#pragma once
#include <atomic>

/// Lock free smart ptr similar to shared ptr.
///	- Destructor of pointed object must not throw. Or operators =, == have undefined behavior.
///	- Published as single header file for readability.
///
///	Formatting: Using sneak_case as stl. This sample takes method signatures from stl, so does casing.
///
/// Known limits:
///	- Owned object is not part of control block.
/// - No custom deleter or allocator.
///	- No separate template type for constructors. (std::shared_ptr constructor has another template type Y)
///	- No make_shared function.
///	- No std::hash<std::shared_ptr>
///	- No std::atomic<std::shared_ptr>
///	- No enable_shared_from_this
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
		/// Control block is always created by a shared ptr. Now weak_ptr alone can create control_block.
		/// All shared pointers collectively have one weak pointer so they keep control block "alive".
		std::atomic<int> weak_usages_{1}; 
		T* payload_{nullptr};
	};

	control_block* control_{nullptr};

	void finish_one_instance_()
	{
		if (!control_)
		{
			return;
		}
		if (--control_->usages_ == 0)
		{
			// Last strong owner.
			// There might still be another (thread with) std::weak_ptr pointing to our control_block.
			delete control_->payload_;
			if (--control_->weak_usages_ == 0)
			{
				delete control_;
			}
		}
	}

	friend void swap(shared_ptr& lhs, shared_ptr& rhs) noexcept
	{
		std::swap(lhs.control_, rhs.control_);
	}

public:
	constexpr shared_ptr() noexcept = default;

	constexpr explicit shared_ptr(nullptr_t) noexcept{}

	explicit shared_ptr(T* ptr)
	try
		: control_(ptr ? new control_block(ptr) : nullptr)
	{
	}
	catch(...)
	{
		delete ptr;
		throw;
	}

	explicit shared_ptr(std::unique_ptr<T,  std::default_delete<T>>&& ptr)
		: control_(new control_block(ptr.release()))
	{
	}

	~shared_ptr() noexcept
	{
		finish_one_instance_();
	}

	shared_ptr(const shared_ptr& other) noexcept
		: control_{other.control_}
	{
		if(other)
		{
			// here at least one valid shared ptr exists. No need to check usages_ for zero.
			++control_->usages_;
		}
	}

	shared_ptr(shared_ptr&& other) noexcept
	{
		std::swap(control_, other.control_);
	}

	template< class Y >
	explicit shared_ptr( const weak_ptr<Y>& r )
		: control_(r.control_)
	{
		int usages = control_->usages_.load();
		do{
			if (usages == 0)
			{
				throw std::bad_weak_ptr{};
			}
		} while(control_->usages_.compare_exchange_weak(usages, 1 + usages));
	}

	// This = operator works for both l-value and r-value.
	shared_ptr& operator=(shared_ptr<T> other) noexcept
	{
		using std::swap;
		swap(*this, other); 
		return *this;
	}

	friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs)
	{
		return lhs.get() == rhs.get();
	}

	friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs)
	{
		return !(lhs == rhs);
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
	friend void swap(weak_ptr& lhs, weak_ptr& rhs) noexcept
	{
		std::swap(lhs.control_, rhs.control_);
	}

	constexpr weak_ptr() noexcept = default;

	~weak_ptr()
	{
		if (control_)
		{
			if (--control_->weak_usages_ == 0)
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
			++control_->weak_usages_;
		}
	}

	// TODO: explicit did not compile
	weak_ptr(const weak_ptr& r) noexcept
	{
		control_ = r.control_;
		if (control_)
		{
			++r.control_->weak_usages_;
		}
	}

	weak_ptr& operator=(weak_ptr other) noexcept
	{
		std::swap(*this, other);
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

	shared_ptr<T> lock()noexcept
	{
		try
		{
			return expired() ? shared_ptr<T>{} : shared_ptr<T>{*this};
		}
		catch (const std::bad_weak_ptr&)
		{
			return shared_ptr<T>{};
		}
	}
};

}
