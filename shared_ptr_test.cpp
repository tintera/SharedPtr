#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "shared_ptr.h"

unsigned int Factorial( unsigned int number ) {
	return number <= 1 ? number : Factorial(number-1)*number;
}

class my_object
{
public:
	static inline std::atomic<int> cnt_{ 0 };

	int id_;

	static void set_seed(const int seed)
	{
		cnt_ = seed;
	}

	my_object()
		: id_(++cnt_)
	{
		std::cout << "Created object:" << id_ << "\n";
	}

	~my_object()
	{
		std::cout << "Deleting object: " << id_ << "\n";
		id_ = 0;
	}

	my_object(const my_object& other)
	{
		const auto old = other.id_;
		id_ = ++cnt_;
		std::cout << "Copying object: " << old << ", new id is:" << id_ << " \n";
	}

	my_object(my_object&& other) noexcept
		: id_{other.id_}
	{
		id_ = ++cnt_;
		//other.id_ = 0; destructor will be called
		std::cout << "Moving from object: " << other.id_ << ", new id is:" << id_ << " \n";
	}

	my_object& operator=(const my_object& other)
	{
		id_ = ++cnt_;
		//other.id_ = 0; destructor will be called
		std::cout << "Assigning from object: " << other.id_ << ", new id is:" << id_ << " \n";
		return *this;
	}

	my_object& operator=(my_object&& other) noexcept
	{
		id_ = ++cnt_;
		//other.id_ = 0; destructor will be called
		std::cout << "Assigning from object: " << other.id_ << ", new id is:" << id_ << " \n";
		return *this;
	}
};


TEST_CASE("Testing empty custom shared ponter.", "[smart_ptr_empty]")
{
	// constexpr smart_ptr::shared_ptr<int> empty_ptr_int{}; // Why then we have constexpr at constructor, when no constexpr instance can be constructed?
	const smart_ptr::shared_ptr<my_object> empty_ptr{};
	REQUIRE( !empty_ptr );
	REQUIRE( empty_ptr.use_count() == 0);
	REQUIRE( empty_ptr.get() == nullptr);
}

TEST_CASE("Create a pointer to object.", "[smart_ptr_owns]")
{
	auto* payload = new my_object();
	const smart_ptr::shared_ptr<my_object> my_ptr(payload);
	REQUIRE( my_ptr );
	REQUIRE( my_ptr.use_count() == 1 );
	REQUIRE( my_ptr.get() == payload );
	REQUIRE( &(*my_ptr) == payload );

	SECTION("Copy construct")
	{
		{
			my_object::set_seed(100);
			const auto my_copy{ my_ptr };  // NOLINT(performance-unnecessary-copy-initialization) // The copy is intentional.
			REQUIRE(my_ptr.use_count() == 2 );
			REQUIRE(my_copy.use_count() == 2 );
			REQUIRE( my_ptr.get() == payload );
			REQUIRE( my_copy.get() == payload );
		}
		REQUIRE(my_ptr.use_count() == 1 );
		REQUIRE( my_ptr.get() == payload );
	}

	SECTION("Copy asignment construct")
	{
		{
			my_object::set_seed(200);
			const auto my_copy = my_ptr;  // NOLINT(performance-unnecessary-copy-initialization) // The copy is intentional.
			REQUIRE(my_ptr.use_count() == 2 );
			REQUIRE(my_copy.use_count() == 2 );
			REQUIRE( my_ptr.get() == payload );
			REQUIRE( my_copy.get() == payload );
		}
		REQUIRE(my_ptr.use_count() == 1 );
		REQUIRE( my_ptr.get() == payload );

	}
}

TEST_CASE("Create a pointer to object and move it.", "[smart_ptr_move]")
{
	SECTION("Move construct")
	{
		auto* payload = new my_object();
		smart_ptr::shared_ptr<my_object> my_ptr(payload);
		smart_ptr::shared_ptr<my_object> moved{ std::move(my_ptr) };
		REQUIRE( my_ptr.use_count() == 0 );  // NOLINT(bugprone-use-after-move) // Intentionally testing moved-from object.
		REQUIRE( moved.use_count() == 1 );
		REQUIRE( my_ptr.get() == nullptr );
		REQUIRE( moved.get() == payload );
	}

	SECTION("Move assign")
	{
		auto* payload = new my_object();
		smart_ptr::shared_ptr<my_object> my_ptr(payload);
		smart_ptr::shared_ptr<my_object> moved = std::move(my_ptr);
		REQUIRE( my_ptr.use_count() == 0 );  // NOLINT(bugprone-use-after-move) // Intentionally testing moved-from object.
		REQUIRE( moved.use_count() == 1 );
		REQUIRE( my_ptr.get() == nullptr );
		REQUIRE( moved.get() == payload );
	}
}

TEST_CASE("Create empty weak ptr.", "[empty_weak_ptr]")
{
	const smart_ptr::weak_ptr<my_object> empty_ptr{};
	REQUIRE(empty_ptr.expired() == true);
}

smart_ptr::weak_ptr<my_object> createExpiredWeakPtr()
{
	auto* payload = new my_object();
	smart_ptr::shared_ptr<my_object> my_ptr(payload);
	smart_ptr::weak_ptr<my_object> weak_ptr(my_ptr);
	return weak_ptr;
}

TEST_CASE("Weak ptr keeps control block.", "[weak_control]")
{
	SECTION("assign")
	{
		auto* payload = new my_object();
		smart_ptr::shared_ptr<my_object> my_ptr(payload);
		REQUIRE(my_ptr.use_count() == 1);
		const smart_ptr::weak_ptr<my_object> weak_ptr(my_ptr);
		REQUIRE(my_ptr.use_count() == 1);
		REQUIRE(my_ptr.get() == payload);
		REQUIRE(weak_ptr.expired() == false);
	}

	SECTION("weak_ptr lock expired")
	{
		auto exp_weak {createExpiredWeakPtr()};
		REQUIRE( exp_weak.expired() == true);
		const smart_ptr::shared_ptr<my_object> shared = exp_weak.lock();
		REQUIRE(shared.get() == nullptr);
	}

	SECTION("weak_ptr lock regular")
	{
		auto* payload = new my_object();
		smart_ptr::shared_ptr<my_object> my_ptr(payload);
		smart_ptr::weak_ptr<my_object> weak_ptr(my_ptr);
		REQUIRE( weak_ptr.expired() == false);
		smart_ptr::shared_ptr<my_object> locked = weak_ptr.lock();
		REQUIRE(locked.use_count() == 2);
		REQUIRE(my_ptr.use_count() == 2);
		REQUIRE(locked.get() == payload);
		REQUIRE(my_ptr.get() == payload);
	}
}
