# "`__x86.get_pc_thunk.ax` referenced in section" link errors

Long question: When I build my own embedded SW and try to link it to the native simulator I get a lot of link errors like
```
`__x86.get_pc_thunk.ax' referenced in section `.text.<my_func>' of <my_obj>: defined in discarded section `.text.__x86.get_pc_thunk.ax[__x86.get_pc_thunk.ax]' of  <my_obj>
`__x86.get_pc_thunk.bx' referenced in section `.text.<my_func>' of <my_obj>: defined in discarded section `.text.__x86.get_pc_thunk.bx[__x86.get_pc_thunk.bx]' of  <my_obj>
```
or the equivalent for `.cx`, `.dx`..

Be sure to build your embedded code **not** aiming at a position independent executable/code; i.e. build with the build options `-fno-pie -fno-pic`


# "`i386:x86-64 architecture of input file <my_obj> is incompatible with i386 output`" link error

Long question: When I build my own embedded SW and try to link it to the native simulator I get a link error like
`i386:x86-64 architecture of input file <my_obj> is incompatible with i386 output`

You are building your embedded code as a 64bit binary, while the runner is built by default as a 32bit one.
Either built the embedded code for x86 (`-m32`), or set the runner to be built as a 64bit one (set `NSI_ARCH=-m64`)
