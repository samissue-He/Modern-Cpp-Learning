#include <iostream>
#include <stdexcept>
#include <concepts> // convertible_to
#include <vector>
#include <complex>
#include <set>

// Simple structure 1
template<typename T>
void printcoll1(const T& coll){
    for(const T& elem : coll){
        std::cout << elem;
    }
}

void printcoll2(auto& coll){
    for(const auto& elem : coll){
        std::cout << elem;
    }
}

// Simple structure 2
template<typename T>
T mymax(T a, T b){ // Copyable in default(complex & atomic are not supported)
    return b < a ? b : a;
}

// Provide explicit error message
template<typename T>
concept haslessthan = requires(T x){{x < x} -> std::convertible_to<bool>;};

template<typename T>
requires std::copyable<T> && haslessthan<T> // type-name
T mymax(T a, T b){
    return b < a ? b : a;
}

// Simple structure 3 --- good application for several typenames using auto as return
template<typename T1, typename T2>
auto choose(T1 a, T2 b){
    return b > a ? a : b;
}

// Confusion by deduction
std::complex<int> c1{1,2};
std::complex c2{1,2}; // Can be deduced

std::vector<int> v2;
std::vector<int> v1{v2.begin(), v2.end()}; // Copy all the elements
std::vector v3{v2.begin(), v2.end()}; // Deduce vector<vector<int>::iterator>, copy its

// NTTP -- Non-Type Template Parameters
template<typename T, int s>
class stack{
    private:
        T elems[s];
    public:
        Stack();
        void push(const T&);
        T pop();
};

stack<int, 20> stack20;
stack<int, 40> stack40;
stack<std::string, 20> stackstring;
// stack20 = stack40 is not allowed

// Variadic Templates
// Launch and process them one by one
template<typename T, typename... Types>
void print_(const T& output, const Types&... args){
    std::cout << output << std::endl;
    print(args...);  // Where's end func?
}

// Solution 
template<typename T, typename... Types>
void print(const T& output, const Types... args){
    std::cout << output << std::endl;
    if constexpr(sizeof...(args)>0){ // compile-time if
        print(args...);
    }
}

// Function parameter --- The same api but different func(easy use)
void add_(auto& coll, const auto& val){
    coll.push_back(val);
}

void add__(auto& coll, const auto& val){
    coll.insert(val);
}

// Solution 1 --- concept
template<typename A>
concept adding = requires(A& coll){coll.push_back();};
void add_c(adding auto& coll, const auto& val){
    coll.push_back(val);
}
void add_c(auto& coll, const auto& val){
    coll.insert(val);
}

// Solution 2 --- if constexpr
void add(auto& coll, const auto& val){
    if constexpr(requires{coll.push_back(val);}){
        coll.push_back(val);
    }else{
        coll.insert(val);
    }
}

std::vector<int> vec1;
std::set<int> vec2; // red black tree

int main(){
    print(1, 2.5, "Hello");
    // That's OK.
    add(vec1, 42); // push_back
    add(vec2, 42); // insert

    exit(EXIT_SUCCESS);
}


