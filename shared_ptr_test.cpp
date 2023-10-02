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
	static inline std::map<int, int> created{};
	static inline std::map<int, int> deleted{};

	int id_;

	static void set_seed(const int seed)
	{
		cnt_ = seed;
	}
	static void reset()
	{
		cnt_ = 0;
	}

	my_object()
		: id_(++cnt_)
	{
		std::cout << "Created object:" << id_ << "\n";
		++created[id_];
	}

	~my_object()
	{
		std::cout << "Deleting object: " << id_ << "\n";
		++deleted[id_];
		id_ = -1;
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

	int id() const
	{
		return id_;
	}
};


TEST_CASE("Testing empty custom shared ponter.")
{
	SECTION("Defalt construct")
	{
		// constexpr smart_ptr::shared_ptr<int> empty_ptr_int{}; // Why then we have constexpr at constructor, when no constexpr instance can be constructed?
		const smart_ptr::shared_ptr<my_object> empty_ptr{};
		REQUIRE( !empty_ptr );
		REQUIRE( empty_ptr.use_count() == 0);
		REQUIRE( empty_ptr.get() == nullptr);
	}
	SECTION("null_ptr construct")
	{
		const smart_ptr::shared_ptr<my_object> empty_ptr{nullptr};
		REQUIRE( !empty_ptr );
	}
	SECTION("Empty ptr construct")
	{
		my_object *ptr{};
		const smart_ptr::shared_ptr<my_object> empty_ptr{ptr};
		REQUIRE( !empty_ptr );
	}
}

TEST_CASE("Create a pointer to object.")
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
			REQUIRE( my_ptr.use_count() == 2 );
			REQUIRE( my_copy.use_count() == 2 );
			REQUIRE( my_ptr.get() == payload );
			REQUIRE( my_copy.get() == payload );
		}
		REQUIRE( my_ptr.use_count() == 1 );
		REQUIRE( my_ptr.get() == payload );

	}

	SECTION("Copy operation removed former object")
	{
		{
			// Can we test proper release of my object?
			my_object::set_seed(300);
			auto my_copy = smart_ptr::shared_ptr<my_object>(new my_object());
			REQUIRE(my_copy.use_count() == 1);
			REQUIRE(my_copy->id() == 301);
			my_copy = my_ptr;  // NOLINT(performance-unnecessary-copy-initialization) // The copy is intentional.
			REQUIRE( my_ptr.use_count() == 2 );
			REQUIRE( my_copy.use_count() == 2 );
			REQUIRE( my_ptr.get() == payload );
			REQUIRE( my_copy.get() == payload );
			REQUIRE( my_object::deleted[301] == 1);
		}
		REQUIRE( my_ptr.use_count() == 1 );
		REQUIRE( my_ptr.get() == payload );

	}

}

TEST_CASE("Create a pointer to object and move it.")
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

TEST_CASE("Equality")
{
	auto* payload = new my_object();
	const smart_ptr::shared_ptr<my_object> my_ptr(payload);
	const smart_ptr::shared_ptr<my_object> emptyA{};
	const smart_ptr::shared_ptr<my_object> emptyB{};

	SECTION("Equal")
	{
		REQUIRE(emptyA == emptyB);
		REQUIRE(my_ptr == my_ptr);
		const auto assigned = my_ptr;  // NOLINT(performance-unnecessary-copy-initialization) // Copy is intentional here.
		REQUIRE(assigned == my_ptr);
		const auto copy{my_ptr};  // NOLINT(performance-unnecessary-copy-initialization) // Copy is intentional here.
		REQUIRE(copy == my_ptr);
	}
	SECTION("Not equal")
	{
		REQUIRE(my_ptr != emptyA);
	}
}

TEST_CASE("Create empty weak ptr.")
{
	const smart_ptr::weak_ptr<my_object> empty_ptr{};
	REQUIRE(empty_ptr.expired() == true);
}

smart_ptr::weak_ptr<my_object> createExpiredWeakPtr()
{
	auto* payload = new my_object();
	// ReSharper disable once CppLocalVariableMayBeConst // Creating weak_ptr is changing shared state and also my_ptr this way.
	smart_ptr::shared_ptr<my_object> my_ptr(payload);
	smart_ptr::weak_ptr<my_object> weak_ptr(my_ptr);
	return weak_ptr;
}

TEST_CASE("Weak ptr keeps control block.")
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

struct MyObjectDeleterFunctor {  
	void operator()(my_object* ptr) {
		std::cout << "custom deleter for my_object id: " << ptr->id(); 
	}
};

TEST_CASE("Converting unique pointer to shared_ptr")
{

	SECTION("weak_ptr lock regular")
	{
		my_object::set_seed(0);
		auto uni = std::make_unique<my_object>();
		const smart_ptr::shared_ptr<my_object> shared(std::move(uni));
		REQUIRE(shared->id() == 1);
	}

	//This tests unique pointer with non-default deleter is not accepted (compilation error)
	//SECTION("Custom deleter does not compile")
	//{
	//	std::unique_ptr<my_object, MyObjectDeleterFunctor> uni(new my_object());
	//	const smart_ptr::shared_ptr<my_object> shared(std::move(uni));
	//}
}


