# linc_process

Haxe/hxcpp native bindings for [tiny-process-library](https://gitlab.com/eidheim/tiny-process-library). This is a [linc](http://snowkit.github.io/linc/) library.

The API provides a simple cross-platform way to create and manage processes in Haxe with C++ target.

## Basic usage

```haxe
import process.Process;

class Main {

    static function main() {

        // Create a new process instance
        final proc = new Process('echo', ['Hello World']);

        // Set up stdout handler
        proc.read_stdout = data -> {
            trace('Output: $data');
        };

        // Start the process
        proc.create();

        // Wait for completion and get exit status
        final status = proc.tick_until_exit_status();
        trace('Process exited with status: $status');

    }
}
```

## Advanced usage

### Writing to process input

```haxe
final proc = new Process('cat');

// Enable stdin
proc.open_stdin = true;

// Set up stdout handler
proc.read_stdout = data -> {
    trace('Received: $data');
};

// Create the process
proc.create();

// Write data to stdin
proc.write('Hello from Haxe!\n');

// Close stdin when done writing
proc.close_stdin();
```

### Setting environment variables

```haxe
final proc = new Process('printenv');

// Set custom environment variables
proc.env = [
    "MY_VAR" => "my_value",
    "ANOTHER_VAR" => "another_value"
];

proc.read_stdout = data -> trace(data);
proc.create();
proc.tick_until_exit_status();
```

### Using custom working directory

```haxe
final proc = new Process('ls');
proc.cwd = "/path/to/directory";
proc.read_stdout = data -> trace(data);
proc.create();
proc.tick_until_exit_status();
```

### Handling both stdout and stderr

```haxe
final proc = new Process('some_command');

proc.read_stdout = data -> {
    trace('stdout: $data');
};

proc.read_stderr = data -> {
    trace('stderr: $data');
};

proc.create();
proc.tick_until_exit_status();
```

### Process cleanup handlers

```haxe
final proc = new Process('long_running_process');

proc.on_stdout_close = () -> {
    trace('stdout stream closed');
};

proc.on_stderr_close = () -> {
    trace('stderr stream closed');
};

proc.create();
proc.tick_until_exit_status();
```

### Tick function when waiting for exit status

When waiting for the process exit status, you can provide a `tick` function that will be called at regular intervals. This can be useful for performing custom processing in parallel while the process is running.

```haxe
proc.tick_until_exit_status(() -> {
    trace('ticking...');
}, 0.01);
```

### Threading

A process can be started from any Haxe thread, allowing you to run multiple processes in parallel using threads. However, each `Process` instance should only be used on the thread where it was created.

All callbacks are called on the same thread as the one used to create the process too.

### Cleaning up process resources

The process is automatically cleaned up when the `Process` instance is garbage collected, but it is good practice to call `proc.destroy()` to release the resources as soon as you don't need them anymore.

