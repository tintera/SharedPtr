## Detailed version (owner count and weak count)
### Create one shared_ptr and one weak_ptr

```plantuml
@startuml
skinparam backgroundColor #EEEBDC
skinparam handwritten true
box "threads"
participant "any" as any <<thread>>
participant "A" as A <<thread>>
participant "B" as B <<thread>>
end box
participant "sp_A" as sp_A <<shared_ptr>>
any    ->  sp_A     ** : make_shared<typename>(params)
sp_A -> sp_A ++ #AFA : <<constructor>>
box "counters methods can be called in multiple threads"
participant "shared count" as cnt <<sp_A's private>>
sp_A ->  cnt ** #FFA: << create >>
participant "weak count" as wcnt <<sp_A's private>>
end box
sp_A ->  wcnt ** #FFA: << create >>
participant "managed object" as mo
sp_A ->  mo ** : << create >>(params) 
participant wp as wp <<weak_ptr>>
sp_A -> any --: shared_ptr instance
any  -> wp ** : <<create>>(sp_A)
wp   -> wp ++ #AFA: <<constructor>>
wp   -> wcnt : atomic increment
wp   -> any --: done
@enduml
```

### weak_ptr.lock() while destroing last shared_ptr

```plantuml
@startuml
skinparam backgroundColor #EEEBDC
skinparam handwritten true
box "threads"
participant "any" as any <<thread>>
participant "A" as A <<thread>>
participant "B" as B <<thread>>
end box
participant "sp_A" as sp_A <<shared_ptr>>
box "counters methods can be called in multiple threads"
participant "shared count" as cnt <<sp_A's private>>
participant "weak count" as wcnt <<sp_A's private>>
participant "managed object" as mo
end box
participant wp as wp <<weak_ptr>>
par
alt
    A    -> sp_A !! : <<destroy>>
    sp_A -> sp_A ++ #FFA : <<destructor>>
    sp_A -> cnt ++ #FFA : atomic decrement
    return 0 owners, 1 weak_ptr
    sp_A -> mo !!: <<destroy>>
    sp_A -> A --: done

    B    -> wp ++ #AAF: lock() 
    participant "empty" as empty <<shared_ptr>>
    wp   -> cnt ++ #AAF : atomic increment (if not zero)
    return : fail (0 owners)
    wp   -> empty ** #AAF : create
    return : empty<<shared_ptr>>
else
    B    -> wp ++ #AAF: lock() 
    participant "sp_B" as sp_B <<shared_ptr>>
    wp   -> cnt ++ #AAF : atomic increment (if not zero)
    return : 2 owners
    wp   -> sp_B ** #AAF : <<create>> 
    return : sp_B <<shared_ptr>>

    A    -> sp_A !! : <<destroy>>
    sp_A -> sp_A ++ #FFA : <<destructor>>
    sp_A -> cnt ++ #FFA : atomic decrement
    return 1 (an other owner exist)
    sp_A -> A --: done
end
end
@enduml
```



### Example: 
- Shared_count keeps number of shared_ptrs pointing to the managed object.
- Weak_count keeps number of weak_ptrs pointing to the managed object.
- Shared_count and weak_count are stored in an object acessible from shared_ptr and weak_ptr.

```plantuml
@startuml
skinparam backgroundColor #EEEBDC
skinparam handwritten true
box "threads"
participant A
participant B
end box
participant shared_ptr as shared
participant weak_ptr as weak
box "shared content can be acessed from multiple threads in paralel"
participant "shared_count" as shared_cnt
participant "weak_count" as weak_cnt
note over shared_cnt
shared_count starts as 1
(one shared_ptr exists)
end note
participant "weak_count" as weak_cnt
note over weak_cnt
weak_count starts as 1 
(one weak_ptr exists)
end note
participant "managed object" as managed
end box
par
A -> weak ++ #FFA: <<destroy>>
weak -> shared_cnt ++ #FFA: is zero?
return no
B -> shared  ++ #AAF: <<destroy>>
shared -> shared_cnt : decrement (sets to 0)
shared -> managed !!: <<destroy>>
shared -> weak_cnt ++ #AAF: is zero?
return no
shared -> B -- : <<done>>
destroy shared 
weak -> weak_cnt ++ #FFA : decrement (sets to 0)
return success
weak -> A : <<done>>
destroy weak
end
note over shared_cnt, weak_cnt
Now both pointers are deleted, 
and counters are dangling.
end note
@enduml
```
Note: If shared count and weak_count are separately allocated objects, they can be destructed one by one.
But then weak_ptr.lock() needs to refer shared_count. And it can be destructed at the time.

### Solution: keep one weak count celectively for all strong counts.
Let's examine destruction of last shared_ptr and weak_ptr.

```plantuml
@startuml
skinparam backgroundColor #EEEBDC
skinparam handwritten true
box "threads"
participant A
participant B
end box
participant shared_ptr as shared
participant weak_ptr as weak
box "shared content can be acessed from multiple threads in paralel"
participant "shared_count" as shared_cnt
note over shared_cnt
shared_count starts as 1
(one shared_ptr exists)
end note
participant "weak_count" as weak_cnt
note over weak_cnt
weak_count starts as 2 
(one weak_ptr exists and
1 is added for all shared_ptrs 
collectively)
end note

participant "managed object" as managed
end box
par
A -> weak ++ #FFA: <<destroy>>
weak -> weak_cnt ++ #FFA : decrement (if not zero) (sets to 1)
return 1
B -> shared  ++ #AAF: <<destroy>>
shared -> shared_cnt ++ #AAF: decrement (if not 0) (sets to 0)
return 0
shared -> managed !!: <<destroy>>
shared -> weak_cnt ++ #AAF: decrement (if not zero) (sets to 0)
return 0
shared -> B -- : <<done>>
destroy shared 
weak -> shared_cnt !!
weak -> weak_cnt !!
weak -> A : <<done>>
destroy weak
end
note over shared_cnt, weak_cnt
Correct clenup.
end note
@enduml
```
It's not important if thread A or theread B sets weak count to 0.
It's destroyed by whichever set's it to zero.
Notes: During implementation we can use method for decrement and destroy if zero.
But preffer non-member function because later we can use allocators and they are not known to counters)

