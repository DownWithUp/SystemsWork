# SystemsWork
A repo containing examples relating to various aspects of Windows internals and processor features.

## Repo Structure
<hr>

### Windows Drivers
* WDM
    * FASMDriver [Here](#fasmdriver)

### Windows Applications
* Native Subsystem Applications
    * Keyboard read and display write [Here](#native-read-write)

### Hypervisors
* WHP (Windows Hypervisor Platform)
    * Event Injection [Here](#whp-event-injection)
* ~~KVMAPI~~

### ~~Intel Processor Trace~~
* ~~Single Range Output and IP Filtering~~


## Example Details
<hr>

### WHP Event Injection
Using the Windows Hypervisor Platform, this exmaple shows how you can inject a CPU event into the guest software via the WHvRegisterPendingEvent register. This register essentially correlates to the VM-entry interruption-information field. The guest software is a pseudo OS, only designed to handle one interrupt. event_os.asm is built with [FASM](https://flatassembler.net).
<hr>

### FASMDriver
A simple example of a WDM hello world driver using [FASM](https://flatassembler.net) and many of its useful macros.
<hr>

### Native Read-Write
A simple example of a native subsystem app which uses NtDisplayString and NtReadFile to write output to the console, and collect input from the keyboard device during the system boot process. Native applications can be launched during the boot process by smss.exe. This example suspends the smss process in order to wait for the user to process the 'C' key to continue the startup process.
<hr>

