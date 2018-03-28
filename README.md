# Windows IME on WSL with X.
On WSL(Windows Subsystem for Linux), you can run X Window applications with X Server like Xming or VcXsrv.
Xming and VcXsrv support Linux IM, but not Windows IME.
So, we must have used Linux IM Engine and Windows IMEditor together.  
This tool helps you to use Windows IME in common.

# stuff
- fcitx-wslime.so
This is fcitx wslime addon. run bridge.exe.

- bridge.exe
This is like npiperelay, redirect STDIN/STDOUT to a named pipe.
And it loads injector.dll.

- injector.dll
This is injected to VcXsrv X window, interprets IME Window Messages and communicates with fcitx-wslime.so.

# limit
- status
WIP: Works a little. No useful.

