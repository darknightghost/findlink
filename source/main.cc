#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <getopt.h>

/**
 * @brief       Print usage.
 *
 * @param[in]   name        Command name.
 */
void usage(const char *name)
{
    printf("Usage:\n"
           "    %s TARGET SEARCH_DIR\n"
           "    %s -h\n"
           "\n"
           "Search symbol links point to the target.\n"
           "\n"
           "Optional Arguments:\n"
           "    -h, --help           Show this help.\n"
           "\n"
           "Positional Arguments:\n"
           "    TARGET               Target of links.\n"
           "    SEARCH_DIR           Directory to search.\n",
           name, name);
}

/**
 * @brief       Do search.
 *
 * @param[in]   target      Link target.
 * @param[in]   searchDir   Search directory.
 *
 * @return      Exit code.
 */
int doSearch(const ::std::filesystem::path &target,
             const ::std::filesystem::path &searchDir)
{
    // Check exists.
    if (! ::std::filesystem::exists(searchDir)) {
        fprintf(stderr, "\"%s\" does not exists.\n", searchDir.c_str());
        return 1;
    }

    struct SearchTask {
        const ::std::filesystem::path &target;
        ::std::filesystem::path        searchDir;
    };
    ::std::mutex                                queueLock;
    ::std::condition_variable                   queueCond;
    ::std::queue<::std::unique_ptr<SearchTask>> taskQueue;

    // Search task.
    auto searchTaskFunc = [&](const ::std::filesystem::path &target,
                              ::std::filesystem::path searchDir) -> void {
        // Check type.
        if (! ::std::filesystem::is_directory(searchDir)) {
            if (::std::filesystem::is_symlink(searchDir)) {
                if (::std::filesystem::absolute(
                        ::std::filesystem::read_symlink(searchDir))
                    == target) {
                    printf("%s\n", searchDir.c_str());
                    return;
                }
            } else {
                return;
            }
        }

        // Search directory.
        for (auto &entry : ::std::filesystem::directory_iterator(searchDir)) {
            if (entry.is_symlink()) {
                // Check.
                if (::std::filesystem::absolute(
                        ::std::filesystem::read_symlink(entry.path()))
                    == target) {
                    printf("%s\n", searchDir.c_str());
                    return;
                }
            } else if (entry.is_directory()) {
                // Add new task.
                ::std::unique_lock<::std::mutex> lock(queueLock);
                taskQueue.push(::std::make_unique<SearchTask>(
                    ::std::ref(target), entry.path()));
                queueCond.notify_one();
            }
        }
    };

    // Search thread.
    ::std::atomic<uint64_t> runningCount
        = ::std::thread::hardware_concurrency();
    auto searchThreadFunc = [&]() -> void {
        while (true) {
            // Get task.
            ::std::unique_ptr<SearchTask> task;
            {
                ::std::unique_lock<::std::mutex> lock(queueLock);
                if (taskQueue.size() > 0) {
                    task = ::std::move(taskQueue.front());
                    taskQueue.pop();
                } else {
                    --runningCount;
                    if (runningCount == 0) {
                        queueCond.notify_one();
                        return;
                    } else {
                        queueCond.wait(lock);
                        ++runningCount;
                        continue;
                    }
                }
            }

            // Search.
            searchTaskFunc(task->target, task->searchDir);
        }
    };

    // Add first task.
    taskQueue.push(
        ::std::make_unique<SearchTask>(::std::ref(target), searchDir));

    // Create threads.
    ::std::vector<::std::thread> threads;
    for (auto i = ::std::thread::hardware_concurrency(); i > 0; --i) {
        threads.push_back(::std::thread(searchThreadFunc));
    }

    // Join.
    for (auto iter = threads.rbegin(); iter != threads.rend(); ++iter) {
        iter->join();
    }

    return 0;
}

/**
 * @brief       Entery.
 *
 * @param[in]   argc        Count of arguments.
 * @param[in]   argv        Values of arguments.
 *
 * @return      Exit code.
 */
int main(int argc, char *argv[])
{
    // Parse arguments.
    struct option longOpts[]
        = {{"help", 0, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "h", longOpts, nullptr)) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                return 0;

            default:
                fprintf(stderr, "Unknow option.\n");
                usage(argv[0]);
                return 0;
        }
    }

    ::std::filesystem::path target;
    ::std::filesystem::path searchDir;
    switch (argc - optind) {
        case 0:
            fprintf(stderr, "Missing argumet \"TARGET\".\n");
            usage(argv[0]);
            return 1u;

        case 1:
            fprintf(stderr, "Missing argumet \"SEARCH_DIR\".\n");
            usage(argv[0]);
            return 1;

        case 2:
            target    = ::std::filesystem::absolute(argv[optind]);
            searchDir = ::std::filesystem::absolute(argv[optind + 1]);
            break;

        default:
            fprintf(stderr, "Too much arguments.\n");
            usage(argv[0]);
            return 1;
    }

    return doSearch(target, searchDir);
}
