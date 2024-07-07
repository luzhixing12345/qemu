import gdb

class ReverseBacktrace(gdb.Command):
    """Print backtrace with function names and file paths in reverse order, with indentation."""

    def __init__(self):
        super(ReverseBacktrace, self).__init__("reverse-bt", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        frame = gdb.newest_frame()
        frames = []

        while frame is not None:
            function_name = frame.name()
            sal = frame.find_sal()
            file_path = sal.symtab.filename if sal.symtab else "Unknown"

            frames.append((function_name, file_path))
            frame = frame.older()

        depth = 0
        for depth, (func_name, file_path) in enumerate(reversed(frames)):
            indent = "  " * depth
            depth += 1
            print(f"{indent}{func_name} [{file_path[3:]}]")

ReverseBacktrace()