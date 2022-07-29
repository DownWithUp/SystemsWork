# IntelSystemsWork
A repo containing examples on how to use various Intel processor features and extensions

## Repo Structure
<hr>

### Hypervisors
* WHP (Windows Hypervisor Platform)
    * Event Injection [Here](#whp-event-injection)
* KVMAPI

### Intel Processor Trace
* Single Range Output and IP Filtering


## Example Details
<hr>

### WHP Event Injection
Using the Windows Hypervisor Platform, this exmaple shows how you can inject a CPU event into the guest software via the WHvRegisterPendingEvent register. This register essentially correlates to the VM-entry interruption-information field. The guest software is a pseudo OS, only designed to handle one interrupt. event_os.asm is built with [FASM](https://flatassembler.net).
<hr>

