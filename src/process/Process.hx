package process;

import haxe.ds.StringMap;
import haxe.io.Bytes;
import haxe.io.BytesData;

class Process {

    /**
     * The command to execute.
     */
    public var command:String;

    /**
     * Array of command line arguments to pass to the process.
     * Automatically shell-escaped.
     */
    public var arguments:Array<String>;

    /**
     * Working directory where the process will be executed.
     * If empty, uses the current working directory.
     */
    public var cwd:String;

    /**
     * Environment variables to pass to the process.
     * If null, inherits the parent process environment.
     */
    public var env:Map<String,String>;

    /**
     * Callback function that receives stdout data as it becomes available.
     * The data parameter contains the output text.
     */
    public var read_stdout:(data:String)->Void = null;

    /**
     * Callback function that receives stderr data as it becomes available.
     * The data parameter contains the error output text.
     */
    public var read_stderr:(data:String)->Void = null;

    /**
     * Whether to open a pipe for writing to the process's stdin.
     * Must be set to true if you want to write input to the process.
     */
    public var open_stdin:Bool = false;

    /**
     * Set to true to inherit file descriptors from parent process. Default is false.
     * On Windows: has no effect unless read_stdout==nullptr, read_stderr==nullptr and open_stdin==false.
     */
    public var inherit_file_descriptors:Bool = false;

    /**
     * If true, detach the process from the parent. When detaching:
     * - On Unix: process runs in new session via setsid()
     * - On Windows: process runs in new session via DETACHED_PROCESS
     * Note: stdin/stdout/stderr callbacks won't work with detached processes
     */
    public var detach_process:Bool = false;

    /**
     * Buffer size for reading stdout and stderr. Default is 131072 (128 kB).
     */
    public var buffer_size:Int = 131072;

    /**
     * If set, invoked when process stdout is closed.
     * This call goes after last call to read_stdout().
     */
    public var on_stdout_close:()->Void = null;

    /**
     * If set, invoked when process stderr is closed.
     * This call goes after last call to read_stderr().
     */
    public var on_stderr_close:()->Void = null;

    /**
     * The handle referencing the native process.
     */
    public var handle(default,null):Int = -1;

    /**
     * If true, this process will inherit environment variables of the parent.
     */
    public var inherit_env:Bool = true;

    /**
     * Creates a new Process instance.
     */
    public function new(?command:String, ?arguments:Array<String>, ?cwd:String, ?env:Map<String,String>) {
        this.command = command;
        this.arguments = arguments ?? [];
        this.cwd = cwd;
        this.env = env ?? new Map();

        #if !cppia
        cpp.vm.Gc.setFinalizer(this, cpp.Function.fromStaticFunction(_finalize));
        #end
    }

    /**
     * Create the actual underlying process with all the parameters previously set.
     */
    public function create():Void {

        var cmd = new StringBuf();
        cmd.add(command);

        if (arguments != null) {
            for (i in 0...arguments.length) {
                var arg = arguments[i];
                #if windows
                arg = haxe.SysTools.quoteWinArg(arg, false);
                #else
                arg = haxe.SysTools.quoteUnixArg(arg);
                #end
                cmd.addChar(' '.code);
                cmd.add(arg);
            }
        }

        var finalEnv = new Map<String,String>();
        if (inherit_env) {
            for (key => val in Sys.environment()) {
                finalEnv.set(key, val);
            }
        }
        if (env != null) {
            for (key => val in env) {
                finalEnv.set(key, val);
            }
        }

        handle = Process_Extern.create_process(
            cmd.toString(),
            cwd ?? Sys.getCwd(),
            finalEnv,
            read_stdout,
            read_stderr,
            open_stdin,
            inherit_file_descriptors,
            detach_process,
            buffer_size,
            on_stdout_close,
            on_stderr_close
        );

    }

    /**
     * Write the given bytes to stdin.
     */
    public extern inline overload function write(bytes:Bytes):Bool {

        return write_bytes(bytes);

    }

    /**
     * Write the given string to stdin.
     */
    public extern inline overload function write(str:String):Bool {

        return write_string(str);

    }

    private function write_bytes(bytes:Bytes):Bool {

        return Process_Extern.write_bytes(handle, bytes.getData(), 0, bytes.length);

    }

    private function write_string(str:String):Bool {

        return Process_Extern.write_string(handle, str);

    }

    /**
     * Close stdin. If the process takes parameters from stdin, use this to notify that all parameters have been sent.
     */
    public function close_stdin():Void {

        return Process_Extern.close_stdin(handle);

    }

    /**
     * Kill the process. force=true is only supported on Unix-like systems.
     */
    public function kill(force:Bool = false):Void {

        return Process_Extern.kill(handle, force);

    }

    #if !windows

    /**
     * Send the signal signum to the process. Only supported on Unix-like systems.
     */
    public function signal(signum:Int):Void {

        return Process_Extern.signal(handle, signum);

    }

    #end

    /**
     * Tick periodically on the current thread, until process is finished, and return exit status.
     */
    public function tick_until_exit_status(?tick:()->Void, tickInterval:Float = 0.001):Int {

        return Process_Extern.tick_until_exit_status(
            handle, tick, Math.round(tickInterval * 1000)
        );

    }

    public function destroy() {

        if (handle == -1) return;

        Process_Extern.remove_process(handle);
        handle = -1;

    }

    @:noCompletion
    @:keep private function _keep():Array<Any> {
        // Just to ensure haxe will include haxe.ds.StringMap in generated C++
        final map = new StringMap<String>();
        return [map];
    }

    @:noCompletion
    @:void public static function _finalize(proc:Process):Void {

        proc.destroy();

    }

}

@:keep
#if !display
@:build(linc.Linc.touch())
@:build(linc.Linc.xml('process'))
#end
@:include('linc_process.h')
@:noCompletion extern class Process_Extern {

    @:native("linc::process::create_process")
    static function create_process(
        command:String,
        path:String,
        env:haxe.ds.StringMap<String>,
        read_stdout:(data:String)->Void,
        read_stderr:(data:String)->Void,
        open_stdin:Bool,
        inherit_file_descriptors:Bool,
        detach_process:Bool,
        buffer_size:Int,
        on_stdout_close:()->Void,
        on_stderr_close:()->Void
    ):Int;

    @:native("linc::process::remove_process")
    static function remove_process(handle:Int):Void;

    @:native("linc::process::tick_until_exit_status")
    static function tick_until_exit_status(handle:Int, tick:()->Void, tick_interval_ms:Int):Int;

    @:native("linc::process::write_bytes")
    static function write_bytes(handle:Int, bytes:BytesData, offset:Int, length:Int):Bool;

    @:native("linc::process::write_string")
    static function write_string(handle:Int, str:String):Bool;

    @:native("linc::process::close_stdin")
    static function close_stdin(handle:Int):Void;

    @:native("linc::process::kill")
    static function kill(handle:Int, force:Bool):Void;

    #if !windows

    @:native("linc::process::signal")
    static function signal(handle:Int, signum:Int):Void;

    #end

}
