#include <iostream>
#include <memory>
#include <utility> // move, pair, swap, exchange, *forward -> handle vari rvalue & lvalue

/*
std::pair<int, double> get_market_data() {
    return {1024, 99.85}; 
}
int main() {
    // C++17 
    auto [id, price] = get_market_data();
*/

/*
struct Node {
    int* data;

    Node(Node&& other) noexcept 
        // other.data = data， other.data = nullptr
        : data(std::exchange(other.data, nullptr)) {} 
};
*/

// Class template(Move semantics)
template <typename T>
class resourcemanager{
    private:
        std::unique_ptr<T> ptr;
    
    public:
        explicit resourcemanager(std::unique_ptr<T> resource)
            :ptr(std::move(resource)){
                std::cout << "Resource";
            }
        ~resourcemanager() = default;

        // Move struct/assignment func
        resourcemanager(resourcemanager && other ) noexcept = default;  // rvalues
        resourcemanager& operator=(resourcemanager && other) noexcept = default;

        // Copy struct/assignment func
        resourcemanager(resourcemanager & other) noexcept = delete;  // lvalues
        resourcemanager& operator=(resourcemanager & other) noexcept = delete;

        // business api
        void process(){
            if(ptr){
                std::cout << "";
                execute();
            }
            
        }
        void execute(){
            if(ptr){
                ptr -> process(); // Use child class's api (if exists)
            }
        }

        // Provide visit api
        T* get() const {return ptr.get();}
};

struct algorithm{
    void process(){
        std::cout << "Hello" << std::endl;
    }
    ~algorithm() = default;
};

int main(){
    std::unique_ptr<algorithm> myal;
    // Transfer to resource class's ptr
    resourcemanager<algorithm> manager(std::move(myal));
    manager.process();
}