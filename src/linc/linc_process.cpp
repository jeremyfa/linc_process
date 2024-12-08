//hxcpp include should be first
#include <hxcpp.h>

#include "./linc_process.h"
#include "process.hpp"

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <queue>
#include <stdexcept>

namespace linc {

    namespace process {

        struct ProcessInfo {
            // Process
            std::shared_ptr<TinyProcessLib::Process> proc = nullptr;

            // Function queue
            std::queue<std::function<void()>> funcQueue;
            mutable std::mutex funcQueueMutex;

            // Default constructor
            ProcessInfo() = default;

            // Delete copy operations to prevent accidental copies
            ProcessInfo(const ProcessInfo&) = delete;
            ProcessInfo& operator=(const ProcessInfo&) = delete;
        };

        inline void queue_func(ProcessInfo *info, std::function<void()> fn) {
            std::lock_guard<std::mutex> lock(info->funcQueueMutex);
            info->funcQueue.push(std::move(fn));
        }

        inline bool pop_func(ProcessInfo *info, std::function<void()>& fn) {
            std::lock_guard<std::mutex> lock(info->funcQueueMutex);
            if (info->funcQueue.empty()) {
                return false;
            }
            fn = std::move(info->funcQueue.front());
            info->funcQueue.pop();
            return true;
        }

        std::mutex mapMutex_;
        std::unordered_map<int, std::unique_ptr<ProcessInfo>> processes_;
        std::atomic<int> nextHandle_{1};

        std::function<void()> wrap_void_func(ProcessInfo *info, ::Dynamic fn) {

            if (hx::IsNull(fn)) return nullptr;

            return [info, fn]() {
                queue_func(info, [fn]() {
                    fn.mPtr->__run();
                });
            };
        }

        std::function<void(const char *bytes, size_t n)> wrap_data_func(ProcessInfo *info, ::Dynamic fn) {

            if (hx::IsNull(fn)) return nullptr;

            return [info, fn](const char *bytes, size_t n) {
                // For now, we always treat output as string
                queue_func(info, [fn, bytes, n]() {
                    ::String str = ::String(std::string(bytes, n).c_str());
                    fn.mPtr->__run(str);
                });
            };
        }

        int create_process(
            ::String command,
            ::String path,
            ::haxe::ds::StringMap env,
            ::Dynamic read_stdout,
            ::Dynamic read_stderr,
            bool open_stdin,
            bool inherit_file_descriptors,
            int buffer_size,
            ::Dynamic on_stdout_close,
            ::Dynamic on_stderr_close
        ) {
            // Get next handle
            int handle = nextHandle_++;

            // Create config
            TinyProcessLib::Config config;
            if (buffer_size != -1) {
                config.buffer_size = buffer_size;
            }
            config.inherit_file_descriptors = inherit_file_descriptors;

            // Create process info on heap
            std::unique_ptr<ProcessInfo> info_ptr(new ProcessInfo());
            ProcessInfo* info = info_ptr.get();

            // Store in map
            {
                std::lock_guard<std::mutex> lock(mapMutex_);
                processes_[handle] = std::move(info_ptr);
            }

            // Create environment
            std::unordered_map<std::string, std::string> environment;
            if (!hx::IsNull(env)) {
                ::Array< ::String > keys =  ::__string_hash_keys(env->h);
                int i = 0;
                int len = keys->length;
                while (i < len) {
                    ::String key = keys->__get(i);
                    if (!hx::IsNull(key)) {
                        ::String value = ::__string_hash_get(env->h,key);
                        if (!hx::IsNull(value)) {
                            environment.emplace(
                                std::string(key.c_str()),
                                std::string(value.c_str())
                            );
                        }
                    }
                    i++;
                }
            }

            // Wrap close handlers
            config.on_stdout_close = wrap_void_func(info, on_stdout_close);
            config.on_stderr_close = wrap_void_func(info, on_stderr_close);

            // Store strings locally to ensure they remain valid during Process construction
            std::string cmd_str(command.c_str());
            std::string path_str(path.c_str());

            // Create the actual process
            info->proc = std::make_shared<TinyProcessLib::Process>(
                cmd_str, path_str,
                environment,
                wrap_data_func(info, read_stdout),
                wrap_data_func(info, read_stderr),
                open_stdin,
                config
            );

            return handle;
        }

        void remove_process(int handle) {
            std::lock_guard<std::mutex> lock(mapMutex_);

            processes_.erase(handle);
        }

        ProcessInfo* get_process(int handle) {
            std::lock_guard<std::mutex> lock(mapMutex_);

            return processes_[handle].get();
        }

        int tick_until_exit_status(int handle, ::Dynamic tick, int tick_interval_ms) {
            auto info = get_process(handle);

            return info->proc->tick_until_exit_status(
                [info, tick] {
                    // Flush pending callbacks
                    std::function<void()> fn;
                    while (pop_func(info, fn)) {
                        fn();
                    }

                    // Run custom tick function, if any
                    if (!hx::IsNull(tick)) {
                        tick.mPtr->__run();
                    }
                },
                tick_interval_ms
            );

        }

        bool write_bytes(int handle, ::Array<unsigned char> bytes, int offset, int length) {
            auto info = get_process(handle);

            return info->proc->write(reinterpret_cast<const char*>(&bytes[0] + offset), length);
        }

        bool write_string(int handle, ::String str) {
            auto info = get_process(handle);

            return info->proc->write(str.c_str());
        }

        void close_stdin(int handle) {
            auto info = get_process(handle);

            info->proc->close_stdin();
        }

        void kill(int handle, bool force) {
            auto info = get_process(handle);

            info->proc->kill(force);
        }

        #ifndef _WIN32

        void signal(int handle, int signum) {
            auto info = get_process(handle);

            info->proc->signal(signum);
        }

        #endif

    }

}
