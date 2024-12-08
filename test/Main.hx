package;

import process.Process;

class Main {

    public static function main():Void {

        trace('START PROC');

        final proc = new Process('echo', ['Hello World']);

        proc.read_stdout = data -> {
            trace('DATA: ' + data);
        };

        proc.create();

        trace('STATUS: ' + proc.tick_until_exit_status());

    }

}
