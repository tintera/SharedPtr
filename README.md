# Lock-free readable shared_ptr implementation 
Lock free C++ shared_ptr implementation for learning purposes concentrating on readability and correctness. 

Shared_ptr is one of cornerstones of modern C++. Sure there exist some production-grade implementations of shared_ptr. The are often not much readable even for seasoned professional C++ developers. Sure they are not readable for most novices and students. Also there exists simple tutorial-style implementations. Useful for learning but missing some important parts like weak_ptr or lock-free multithreading for sake of simplicity. This implementation has two goals is trying to take some middle ground. To be more complete than simple tutorials yet also easier to study and read than production libraries. Also I have to admit one goal is to let myself better understand stl::shared_ptr details and lock-fee data structures.

Currently the implementation is not complete (some parts of C++ standard requirements are missing see section known limits and omitted)
It's planned to add them as long as they do not compromise readability.
 
## Expectations 
(not sure if they are part of the standard)
- Destructor of pointed object must not throw. Otherwise operators =, == have undefined behavior.
- Published as single header file for readability.
 
## Known limits:
- Some race condition exist. Best to fix them and keep implementation lock free. And keep default constructor noexcept (as in std::)
- No custom deleter or allocator.
- No separate template type for constructors. (std::shared_ptr constructor has another template type Y)
- No `make_shared` function. (And single allocation for both owned object and of control block)
- No `std::hash<std::shared_ptr>`
- No `std::atomic<std::shared_ptr>`
- No `enable_shared_from_this`

## Omitted
- `reset`
- `swap`
- `operator[]` managing arrays not implemented at all.
- `unique` (as it's removed in C++ 20)
- `owner_before`
- `operator <<(std::shared_ptr)`

## Acknowledgements
Thank you all who helped with this implementation:
TODO: list names/links.
This implementation would be far complete or correct without your help and mainly your code reviews.
