#include <vector>
#include <boost/shared_ptr.hpp>

#include "common/tpool/Mutex.h"
#include "common/tpool/Thread.h"
#include "common/tpool/rcu/test_common.h"

static ::tpool::sync::Mutex gs_foo_guard;
static ::tpool::sync::Mutex gs_sum_guard;

static void MutexReadThreadFunc()
{
    struct Foo* foo = NULL;
    int sum = 0;
    unsigned int i;
    int j;
    for (i = 0; i < LOOP_TIMES; ++i) {
        for (j = 0; j < 1000; ++j) {
            ::tpool::sync::MutexLocker lock(gs_foo_guard);
            foo = gs_foo;
            if (foo) {
                sum += foo->a + foo->b + foo->c + foo->d;
            }   
        }   
    }   
    ::tpool::sync::MutexLocker lock(gs_sum_guard);
    gs_sum += sum;
}

static void MutexWriteThreadFunc()
{
    int i;
    struct Foo* old_foo = NULL;
    while (!gs_is_end) {
        for (i = 0; i < 1000; ++i) {
            struct Foo* foo = new Foo;
            foo->a = 2;
            foo->b = 3;
            foo->c = 4;
            foo->d = 5;

            {
                ::tpool::sync::MutexLocker lock(gs_foo_guard);
                old_foo = gs_foo;
                gs_foo = foo;
            }

            if (old_foo) {
                delete old_foo;
            }
        }
    }
}

int main(int argc, char** argv)
{
    int thread_num = 4;
    if (argc > 1) {
        int n = atoi(argv[1]);
        if (n > 0) {
            thread_num = n;
        }
    }

    gs_foo = new Foo;
    gs_foo->a = 1;
    gs_foo->b = 2;
    gs_foo->c = 3;
    gs_foo->d = 4;

    typedef ::boost::shared_ptr< ::tpool::Thread> ThreadPtr;
    ::std::vector<ThreadPtr> threads;

    for (int i = 0; i < thread_num; ++i) {
        threads.push_back(ThreadPtr(new ::tpool::Thread(&MutexReadThreadFunc)));
    }

    {
        ::tpool::Thread write_t(&MutexWriteThreadFunc);
        threads.clear();
        gs_is_end = true;
    }

    return 0;
}
