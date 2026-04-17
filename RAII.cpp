#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

// Mutex
struct BufferClass
{
    void display() {};
};
struct SomeDataSource
{
    bool readIntobuffer(BufferClass &buffer)
    {
        return true;
    };
};

bool fn(std::mutex &someMutex, SomeDataSource &src)
{
    std::lock_guard lock{someMutex}; // std::unique_lock can make more choices(Need to lock/unlock by hand)
    try
    {
        BufferClass buffer;
        if (not src.readIntobuffer(buffer))
        {
            return false;
        }
        buffer.display();
    }
    catch (...)
    {
        throw;
    }
    return true;
}


// Thread
struct SomeClass
{
    void fnn();
};
void SomeClass::fnn()
{
    auto worker{std::jthread{[] { /* do something*/ }}};
}

void doss(){};
auto worker{std::jthread{doss}}; // Pass the function name (not the call) or a lambda anonymous function.


// File
struct file_closer{  // The parentheses operator () is overloaded; this is called a "functor".
    void operator()(FILE* stream) const{fclose(stream);}
};
using cfile = std::unique_ptr<FILE, file_closer>; // The second parameter can be a structure (functor) or a function pointer; the default is to delete the pointer.
// C++20
using pcfile = std::unique_ptr<FILE, decltype([](FILE* fp){fclose(fp);})>;

auto make_cfile(char const* filename, char const* mode){
    FILE* stream{fopen(filename,mode)};
    if(not stream){
        throw std::runtime_error("Failed to open file");
    }
    return cfile{stream};
}

void fn(){
    auto file{make_cfile("filename.txt", "w")};
    fprintf(file.get(), "Data for the file.");
}

// RAII class usage
template <class Mutex> // Compatible with various types of mutexes
class unique_unlock{
    public:
        explicit unique_unlock(std::unique_lock<Mutex> &p_lock)
            : lock(p_lock){lock.unlock();} // Member initialization, lock is a reference to p_lock. The same as self.name = name.
        ~unique_unlock(){lock.lock();} // When it leaves its scope and is destroyed, it automatically calls lock().
    private:
        std::unique_lock<Mutex> &lock;
};

std::mutex mut;
void fm(){
    std::unique_lock ul{mut}; // ul takes control of mut, others cannot operate mut
    // Do some work protected by the mutex
    {
        unique_unlock u{ul};
        // Do some work not protected
    }
    // The resource was deleted and the work protected by the mutex again
}


int main()
{
    // pointers
    auto ptr1{std::make_unique<float>(10.0f)}; // Create new pointer "make_shared"
    std::cout << *ptr1 << std::endl;
    std::shared_ptr<float> ptr2{std::move(ptr1)}; // Data type "unique_ptr"
    std::cout << *ptr2 << std::endl;
    std::cout << (ptr1 == nullptr ? "correct" : "incorrect") << std::endl;

    return 0;
}