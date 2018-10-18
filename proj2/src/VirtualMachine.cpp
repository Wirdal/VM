#include <VirtualMachine.h>
/*!
VMStart() starts the virtual machine by loading the module specified by argv [0]. The argc and argv
are passed directly into the VMMain() function that exists in the loaded module. The time
in milliseconds of the virtual machine tick is specified by the tickms parameter.
*/
TVMStatus VMStart(int tickms, int argc, char *argv[]){

};
/*!
Upon successful loading and running of the VMMain() function, VMStart() will return
VM_STATUS_SUCCESS after VMMain() returns. If the module fails to load,
or the module does not contain a VMMain() function, VM_STATUS_FAILURE is returned.
*/
