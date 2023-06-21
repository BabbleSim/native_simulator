## Native simulator CPU Start/Stop Emulation (NCE)

The native simulator CPU emulator, is an *optional* module provided by the native simulator which
the hosted embedded OS / SW adaptation layer can use to emulate the CPU being started and stopped.

Its mode of operation is that it step-locks the HW and SW operation, so that only one of
them executes at a time.

This component is always built with the runner, it is up to each embedded CPU SW to use it or not.

If used, an instance component should always be initialized by calling `nce_init()`.
This will return a pointer to an instance ready to use.

Correspondingly, during exit of the simulation, the embedded CPU SW adaptation layer should
call `nce_terminate()` with that same pointer to clean up.

After initialization and during boot (when `nsif_cpu?_boot()` is called) the CPU SW
should call `nce_boot_cpu()` to spawn a new pthread which will be dedicated to the
embedded SW. This new thread will run the embedded SW initializatrion function provided
as paremeter to `nce_boot_cpu()`.

Note that the HW scheduling will be blocked (and therefore time will not pass)
until `nce_halt_cpu()` is called from either that new thread, or another thread it spawns.

Calling `nce_halt_cpu()` blocks the calling SW thread.

After this, the HW models will call `nce_wake_cpu()` whenever the CPU must be awoken (for ex.
due to an interrupt), which will resume the SW execution of the SW thread which was blocked by
calling `nce_halt_cpu()`.

For detailed information on each API function please check the source:
[../common/src/nce.c](../common/src/nce.c)
