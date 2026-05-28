#include <iostream>
#include <execution>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm> // for_each...
#include <numeric>   // transform_reduce...
#include <random>
#include <chrono>             // For time 500ms
#include <condition_variable> // .wait
#include <shared_mutex>
#include <semaphore>

#define SIZE 8192

// Execution policy
void sequential_demo(std::vector<double> &vec)
{
    std::for_each(std::execution::seq, vec.begin(), vec.end(), [](double &x)
                  { x = x * x; });
}

void parallel_demo(std::vector<double> &vec)
{
    std::for_each(std::execution::par, vec.begin(), vec.end(), [](double &x)
                  { x = x * x; });
}

void vectorized_demo(std::vector<double> &vec)
{
    std::for_each(std::execution::unseq, vec.begin(), vec.end(), [](double &x)
                  { x = x * x; });
}

// Transform is default to mapping on a new vec(like using back_inserter())
// But u can repeatly use vec.begin() to utilize revise in the origin vec
// The logic of `std::transform` is to write the element, after being computed using a lambda expression, 
// into the target iterator (the fourth parameter).
// Therefore, the lambda expression must return the computed new value.
void fullspeed_demo(std::vector<double> &vec)
{
    std::transform(std::execution::par_unseq, vec.begin(), vec.end(), vec.begin(), [](double &x)
                   { return x = x * x; });
}

void parallel_strategy()
{
    // Parallel strategy
    // Like using for_each, transform(1 or 2 srcs), sort, reduce
    std::vector<double> vec1(SIZE);
    std::vector<double> vec2(SIZE);
    // Random Initialization
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    for (auto &i : vec1)
    {
        i = static_cast<double>(dis(gen));
    }
    for (auto &i : vec2)
    {
        i = static_cast<double>(dis(gen));
    }
    // A[0]*B[0] + A[1]*B[1] + ...
    auto dot_product = std::transform_reduce(
        std::execution::par_unseq,
        vec1.begin(),
        vec1.end(),
        vec2.begin(),
        0.0,                // Init value
        std::plus<>{},      // Reduce operator
        std::multiplies<>{} // Transform operator
    );
}

// Thread "Avoid using directly"
void sample_thread()
{
    std::thread mythread{[]()
                         {
                             std::cout << "Hello from thread." << std::endl;
                         }};
    // Must use join or detach(Hope to extend thread's life time)
    mythread.detach();
}

// When two thread use the same mutex,
// Thread 1 : .A. m.lock() .B. m.unlock() .C.
// Thread 2 : .......D........ m.lock() .E. m.unlock() .F.
// A & B are visible to E & F

// Avoid data race(Read and write happen at the same time)
// Prefix code
struct transaction
{
    int amount;
};
struct account
{
    int balance;
    std::mutex mutex;
};

int overdraft_charge = 10;
std::string overdraft_message = "Insufficient funds";

// Revise the status of the t(not explain in this code)
transaction overdraft_of(transaction t)
{
    return t;
}

// A constant lvalue reference can be bound to a rvalue.(overdraft_of)
void record(account &acc, const transaction &t) {}
void accept_transaction(transaction &t) {}
void reject_transaction(transaction &t) {}
void notify_user(account &acc, const std::string &msg) {}

// Wrong code --- lead to data race (account.balance in "if" & sentence)
void wrong(transaction &transaction, account &account)
{
    if (transaction.amount < account.balance)
    {
        std::scoped_lock lock(account.mutex);
        account.balance -= transaction.amount;
        record(account, transaction);
        accept_transaction(transaction); // Don't need locked mutex in accept or reject
    }
    else
    {
        std::scoped_lock lock(account.mutex);
        account.balance -= overdraft_charge;
        record(account, overdraft_of(transaction));
        notify_user(account, overdraft_message);
        reject_transaction(transaction);
    }
}

// Right code
void right(transaction &transaction, account &account)
{
    bool trans_ok = transaction.amount < account.balance;
    {
        std::scoped_lock lock(account.mutex);
        if (trans_ok)
        {
            account.balance -= transaction.amount;
            record(account, transaction);
        }
        else
        {
            account.balance -= overdraft_charge;
            record(account, overdraft_of(transaction));
            notify_user(account, overdraft_message);
        }
    }
    if (trans_ok)
    {
        accept_transaction(transaction);
    }
    else
    {
        reject_transaction(transaction);
    }
}

// Deadlock
// Seems like several threads are scramble for a lock
template <typename T, typename... type>
void change_data(T &output, type... args)
{
    std::cout << output;
    if constexpr (sizeof...(args) > 0)
    {
        change_data(args...);
    }
}

// Thread 1 hold mutex_a & Thread 2 hold mutex_b, cannot proceed
void wrongcode(std::mutex &mutex_a, std::mutex &mutex_b)
{
    int data_a, data_b;
    {
        std::scoped_lock la1(mutex_a);
        change_data(data_a);
        {
            std::scoped_lock lb1(mutex_b);
            change_data(data_a, data_b);
        }
    }
    {
        std::scoped_lock lb2(mutex_b);
        change_data(data_b);
        {
            std::scoped_lock la2(mutex_a);
            change_data(data_a, data_b);
        }
    }
}

// `std::scoped_lock` internally uses `std::lock` algorithm to sort memory addresses.
// That means (mutex_a, mutex_b) = (mutex_a, mutex_b)
// It uses a rollback mechanism that will give up holding mutex while others need
void rightcode(std::mutex &mutex_a, std::mutex &mutex_b)
{
    int data_a, data_b;
    {
        std::scoped_lock l1(mutex_a, mutex_b);
        change_data(data_a);
        change_data(data_a, data_b);
    }
    {
        std::scoped_lock l2(mutex_b, mutex_a);
        change_data(data_b);
        change_data(data_b, data_a);
    }
}

// T atomicAdd(T* address, T val) in cuda
// atomic can avoid data race such as
void atomic_sample(double &x)
{
    {
        std::thread threads{[&]()
                            { x + 1; }};
        std::thread threadss{[&]()
                             { x * 2; }};
    }
}

// flag loop for deadlock
// solution --- but do-while exhausts, we can use condition_variable .wait api in modern C++
void deadlockloop()
{
    bool flag = false;
    std::mutex flag_mutex;
    std::thread t([&]()
                  {
        std::cout << "Waiting..." << std::endl;
        bool local_flag;
        do{
            std::scoped_lock lock{flag_mutex};
            local_flag = flag;
        }while(not local_flag);
        std::cout << "Flag changed." << std::endl; });
    // main process sleep and let thread running first
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    {
        std::scoped_lock lock{flag_mutex};
        flag = true;
    }
    t.join();
    std::cout << "Done.\n"
              << std::endl;
}

// If flag is true, skip directly; otherwise, sleep and do not consume CPU resources.
// When the main thread wakes up cv using notify, perform another check.
// The thread run one time only
void good_loop()
{
    bool flag = false;
    std::mutex flag_mutex;
    std::condition_variable cv;
    std::thread th{[&]()
                   {
                       std::cout << "Waiting..." << std::endl;
                       std::unique_lock lock{flag_mutex};
                       cv.wait(lock, [&]() { // .wait must use unique_lock(hand release)
                           return flag;
                       });
                       std::cout << "Flag changed." << std::endl;
                   }};

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    {
        std::scoped_lock lock{flag_mutex};
        flag = true;
    }
    cv.notify_one();
    th.join();
    std::cout << "Done.\n"
              << std::endl;
}

// When revision is too much, we can split into several batches
void batch()
{
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    threads.reserve(100);
    auto increment = [&]() { // An object, which is same as writing in thread{}
        int local_counter = 0;
        for (int i = 0; i < 50; i++)
        {
            ++local_counter;
        }
        counter += local_counter;
    };
    for (int i = 0; i < 100; ++i)
    {
        threads.push_back(std::thread(increment));
    }
    for (std::thread &t : threads)
    {
        t.join();
    }
    std::printf("%d\n", counter.load());
}

// std::recursive_mutex
// Same threads can lock the mutex multiple times

// std::timed_mutex
// try_lock_for or try_lock_until func

// std::shared_mutex
// There are multiple read and write operations. 
// Write operations use a lock, and read operations use a lock_shared. 
// Read operations do not affect each other.
class SharedData{
    private:
        std::shared_mutex rw_mutex;
        double stock_price = 100.0;
    public:
        double get_price(){
            std::shared_lock<std::shared_mutex> read_lock(rw_mutex);
            return stock_price;
        }

        void update_price(double new_price){
            std::scoped_lock write_lock(rw_mutex);
            stock_price = new_price;
            std::cout << "Price updated to: " << stock_price << "\n";
        }
};

// Counting_semaphore
// Permits = 3: Maximum of 3 threads can acquire the semaphore simultaneously
template<typename T>
void worker_task(int id, T& sem){
    sem.acquire();
    std::cout << "I love u." << std::endl;
    sem.release();
}

// The `std::thread` constructor passes parameters by value by default. 
// When passing a reference parameter to `std::thread`, 
// you must explicitly wrap it with `std::ref` to tell the thread to pass it by reference.
int main(){
    // These represent the maximum upper limit of sem and the initial resources, respectively.
    // For example, if the maximum value is 2, and threads A and B both call `sem`,
    // but the initial value is 1,
    // this means that only one of threads A and B can be working at any given time.
    std::counting_semaphore<3> sem{3};
    std::vector<std::thread> threads;
    for(int i=0; i<10; i++){
        threads.push_back(std::thread(worker_task<std::counting_semaphore<3>>, i, std::ref(sem)));
    }
    for(auto& t : threads){
        t.join();
    }
    return 0;
}