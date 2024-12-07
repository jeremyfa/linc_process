#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

namespace linc {

    namespace process {

        extern int create(
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
        );

        extern int get_exit_status(int handle);

        extern bool write_bytes(int handle, ::haxe::io::Bytes bytes, int offset, int length);

        extern bool write_string(int handle, ::String str);

        extern void kill(int handle, bool force);

    }

}
