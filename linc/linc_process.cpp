//hxcpp include should be first
#include <hxcpp.h>

#include "./linc_process.h"
#include "process.hpp"

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <memory>
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

            // Allow move operations
            ProcessInfo(ProcessInfo&&) = default;
            ProcessInfo& operator=(ProcessInfo&&) = default;
        }

        inline void queue_func(ProcessInfo& info, std::function<void()> fn) {
            std::lock_guard<std::mutex> lock(info.funcQueueMutex);
            info.funcQueue.push(std::move(fn));
        }

        inline bool pop_func(ProcessInfo& info, std::function<void()>& fn) {
            std::lock_guard<std::mutex> lock(info.funcQueueMutex);
            if (info.funcQueue.empty()) {
                return false;
            }
            fn = std::move(info.funcQueue.front());
            info.funcQueue.pop();
            return true;
        };

        std::mutex mapMutex_;
        std::unordered_map<int, ProcessInfo> processes_;
        std::atomic<int> nextHandle_{1};

        std::function<void()> wrap_void_func(ProcessInfo& info, ::Dynamic fn) {

            if (hx::IsNull(fn)) return nullptr;

            return [&info, fn]() {
                queue_func(info, [fn]() {
                    fn->__run();
                });
            };
        }

        std::function<void(const char *bytes, size_t n)> wrap_data_func(ProcessInfo& info, ::Dynamic fn) {

            if (hx::IsNull(fn)) return nullptr;

            return [&info, fn](const char *bytes, size_t n) {
                ::String str = ::String(std::string(bytes, n));
                queue_func(info, [fn, str]() {
                    fn->__run(str);
                });
            };
        }

        int create(
            ::String command,
            ::Array<String> arguments,
            ::String path,
            ::haxe::ds::StringMap env,
            ::Dynamic read_stdout,
            ::Dynamic read_stderr,
            bool open_stdin,
            bool inherit_file_fescriptors,
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
            config.inherit_file_descriptors = inherit_file_fescriptors;

            // Create process info
            ProcessInfo* info;
            {
                std::lock_guard<std::mutex> lock(mapMutex_);
                processes_.emplace(handle, ProcessInfo{});
                info = &processes_[handle];
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
            config.on_stdout_close = wrap_void_func(*info, on_stdout_close);
            config.on_stderr_close = wrap_void_func(*info, on_stderr_close);

            // Create the actual process
            info->proc = std::make_shared<TinyProcessLib::Process>(
                command.c_str(), path.c_str(),
                environment,
                wrap_data_func(*info, read_stdout),
                wrap_data_func(*info, read_stderr),
                open_stdin
                config
            );

            return handle;
        }

        int get_exit_status(int handle) {

        }

        bool write_bytes(int handle, ::haxe::io::Bytes bytes, int offset, int length) {

        }

        bool write_string(int handle, ::String str) {

        }

        void kill(int handle, bool force) {

        }

    }

}
