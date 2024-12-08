#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

#include "haxe/ds/StringMap.h"

namespace linc {

    namespace process {

        extern int create_process(
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
        );

        extern void remove_process(int handle);

        extern int tick_until_exit_status(int handle, ::Dynamic tick, int tick_interval_ms);

        extern bool write_bytes(int handle, ::Array<unsigned char> bytes, int offset, int length);

        extern bool write_string(int handle, ::String str);

        extern void close_stdin(int handle);

        extern void kill(int handle, bool force);

        #ifndef _WIN32

        extern void signal(int handle, int signum);

        #endif

    }

}
