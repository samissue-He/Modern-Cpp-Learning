#include <iostream>
#include <stdexcept>
#include <concepts>
#include <vector>
#include <memory>
#include <type_traits>
#include <string>

// The first usage of concept
namespace io
{
    template <class T>
    concept printable = requires(std::ostream &os, T a) {
        a.print(os); // An expression that if compiles yields true
    } or requires(std::ostream &os, T a) {
        a->print(os); // Allow pointer especially smart pointer
    };
}

class my_type
{
    std::string s{"foo\n"};

public:
    void print(std::ostream &os) const
    {
        os << "s:" << s;
    }
    bool operator==(const my_type &) const = default;  // Generate comparison operation autoly
    auto operator<=>(const my_type &) const = default; // evolue to totally_ordered and support all regular
};
static_assert(io::printable<my_type>); // Any print func in the class?

// For func definition
io::printable auto print_(const io::printable auto &s)
{
    return s;
}

// For func overloading (or using "if constexpr", see template)
void print_ln(auto p)
{
    std::cout << p << std::endl;
}

void print_ln(io::printable auto p)
{
    p.print(std::cout);
    std::cout << std::endl;
}

// Non-template member func
template <class T>
class wrapper
{
    T val_;

public:
    wrapper(T val)
        : val_(val) {};
    T operator*()
        requires std::is_pointer_v<T>
    {
        return val_;
    }
};

// Constrains existing templates
template <typename T>
using vec = std::vector<T>;

// Official library has many concept and you can use it by searching
// Following is example
template <typename T>
requires std::floating_point<T> && std::regular<T>
T ins(T value)
{
    return value;
}

auto inst(std::floating_point auto v)
{
    return v;
}

// Regular and semiregular
static_assert(std::regular<my_type>);

// Using range concepts --- Compile-time constraints ensure that the passed-in variable is within a "range".
void print_ints(const std::ranges::range auto &r)
{
    for (const auto &i : r)
    {
        std::cout << i << std::endl;
    }
}

// Concept and CRTP
// Son classed derives its class' types to its father class
// Father class can use son class's api in compile time and deduct new func
template<class D>
requires std::is_class_v<D> && std::same_as<D, std::remove_cv_t<D>>
class view_interface{ 
    // If possible, the compiler will calculate the value directly during the compilation phase, 
    // incurring zero runtime overhead; otherwise, it will run as ordinary code.
    constexpr const D& derived () const noexcept{
        return static_cast<const D&>(*this);
    }

    template<std::random_access_iterator r = const D>
    constexpr decltype(auto) operator[](std::ranges::range_difference_t<r> n) const{
        return std::ranges::begin(derived())[n];
    }
};

template<std::input_iterator V, std::indirect_unary_predicate<std::ranges::iterator_t<V>> Pred>
requires std::ranges::view<V> && std::is_object_v<Pred>
class filter_view : public view_interface<filter_view<V, Pred>>{};


int main()
{
    print_ln("m");
    my_type m;
    print_ln(m);

    // Cooperate with smart pointer
    const io::printable auto upm{std::make_unique<my_type>()};
    upm->print(std::cout);

    auto wi{std::make_unique<wrapper<int>>(1)};
    std::cout << wi.get() << std::endl;

    vec<my_type> vp{}; // The same as std::vector<my_type>
    for (const auto &item : vp)
    {
        item.print(std::cout);
    }

    return 0;
}