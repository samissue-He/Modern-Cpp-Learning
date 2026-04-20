#include <iostream>
#include <memory>
#include <vector>
#include <thread>

// unique_ptr
// 1
struct ResultType
{
};
struct InputType
{
};
struct HelperType
{
    HelperType(InputType) {}
    void calculate(){
        std::cout << "";
    };
    ResultType important_result(){
        return ResultType{};
    };
};
void calculate_more(HelperType &){
    std::cout << "";
};

ResultType do_work(InputType inputs)
{
    auto owner{std::make_unique<HelperType>(inputs)};
    owner->calculate();
    calculate_more(*owner);
    return owner->important_result();
}

// 2
struct widgetBase{
    virtual ~widgetBase() = default;
};
std::unique_ptr<widgetBase> create_widget(InputType){
    return std::make_unique<widgetBase>();
}; // Function declaration that returns a smart pointer type

class Myclass
{
private:
    std::unique_ptr<widgetBase> owner;

public:
    Myclass(InputType inputs)
        : owner(create_widget(inputs)) {}
    ~Myclass() = default;
};

// 3
template <typename T>
struct unique_ptr{
    T* ptr;
    T* release() noexcept{
        T* old = ptr;
        ptr = nullptr;
        return old;
    }
    void reset(T* p=nullptr) noexcept{
        delete ptr;
        ptr = p;
    }
    T* get() const noexcept{
        return ptr;
    }
    explicit operator bool() const noexcept{ // Allows you to define class objects to use certain operators just like built-in types.(like bool(ptr) not bool(ptr.get()!=nullptr))
        return ptr != nullptr;
    }
};


// Aliasing Constructor
// Allows APIs to expose only a portion of an object without losing control over its overall lifecycle.
struct Inner{};
struct outer{
    int a;
    Inner inner;
};

void fake(std::shared_ptr<outer> op){
    std::shared_ptr<Inner> ip(op, &op -> inner); // ip only controls the "inner" member within the outer member.
}
void work(int){
        std::cout << "";
    }
    
int main(){
    // shared_ptr
    // It is not recommended to use it in a multi-threaded environment.
    // 1
    auto a{std::make_shared<int>(42)};
    // lambda
    std::thread t([](std::shared_ptr<int> b){ // Must be put in main
        std::shared_ptr<int> c = b;
        work(*c);
    },a);
    {
        std::shared_ptr<int> d = a;
        a.reset((int*)nullptr);
        work(*d);
    }
    t.join();


    // weak_ptr
    std::weak_ptr<int> w;
    {
        auto s{std::make_shared<int>(42)};
        w = s;
        std::shared_ptr<int> t = w.lock();
        if(t) std::cout << *t;
    }
    std::shared_ptr<int> u = w.lock();
    if(!u) std::cout << "empty";


    // custom deleter
    size_t N;
    char buffer[1024];
    // auto deleter = [](float* p){cudaFree(p);}
    struct fclose_deleter{ // Compared to functors, it has high reusability and small memory footprint.
        void operator()(FILE* fp) const{ fclose(fp); }
    };
    // unique_ptr
    using unique_file = std::unique_ptr<FILE, fclose_deleter>;
    {
        unique_file fp{fopen("readme.txt", "r")};
        if(fp) fread(buffer, 1, sizeof(buffer), fp.get());
    }
    // shared_ptr
    using shared_file = std::shared_ptr<FILE>;
    {
        shared_file fp{fopen("readme.txt","r"),fclose_deleter{}};
        if(fp) fread(buffer, 1, sizeof(buffer), fp.get());
        std::shared_ptr<FILE> fp2{fp};
    }


    // cast
    struct BlueWidget : public widgetBase {
        void do_something_blue(){
            std::cout << "Hello World.";
        };
    };
    InputType inputs;
    std::shared_ptr<widgetBase> p{create_widget(inputs)};
    auto b{std::dynamic_pointer_cast<BlueWidget>(p)};
    if(b) b -> do_something_blue();

    return 0;
}
