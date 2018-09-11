# AprPoolAllocator
C++ wrapper for APR pools

### Using an APR pool

```apr_initialize()``` must be called before using any APR lib and ```apr_terminate()``` when you're done.

### Getting an allocator

Allocators must be initialized from already existing memory pools by calling the specific constructor or with the ```getAllocator()``` method.

```cpp
Pool p;

auto intAllocator1 = p.getAllocator<int>();
auto intAllocator2 = Pool::AprAllocator<int>(p);
```

### Using an allocator with STL containers

```cpp
Pool p;

auto allocator = p.getAllocator<int>();

// all of these are equivalent
std::vector<int, Pool::AprAllocator<int>> v({ 1, 2, 3 }, allocator);
auto vec2 = std::vector<int, decltype(allocator)>({ 4, 5, 6 }, allocator);
decltype(vec1) vec3({ 7, 8, 9 }, allocator););
```
### Attaching/Detaching objects

You can specify a ```cleanup function``` when attaching an already allocated object that will be called when the memory pool it's attached to is destroyed.

```cpp
apr_status_t my_cleanup_function(void *data)
{
  reintepret_cast<MyObject*>(data)->~MyObject();
  delete MyObject;
  return APR_SUCCESS;
}

[...]

Pool p;

MyObject *o = new MyObject():
p.attach(o, (Pool::Callback)my_cleanup_function); //attach
p.detach(o, (Pool::Callback)my_cleanup_function); //unregister the specific cleanup function
```
