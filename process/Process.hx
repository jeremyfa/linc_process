package process;

class Process {

    public function new() {
        // TODO
    }

}

@:keep
#if !display
@:build(linc.Linc.touch())
@:build(linc.Linc.xml('process'))
#end
@:include('linc_process.h')
@:noCompletion extern class Process_Extern {

    // TODO

}
