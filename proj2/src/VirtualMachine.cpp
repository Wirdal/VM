#include <VirtualMachine.h>
#include <Machine.h>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

/*****************************
 * The required code is here *
 * **************************/

extern "C" {
    typedef void (*TVMMainEntry)(int, char*[]); 
    TVMMainEntry VMLoadModule(const char *module);
    void VMUnloadModule(void);
    TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
}


TVMStatus VMStart(int tickms, int argc, char *argv[]){
    MachineInitialize();
    MachineEnableSignals();
    MachineRequestAlarm(1000* tickms, AlarmCallback, NULL);

};

TVMStatus VMTickMS(int *tickmsref){

};
TVMStatus VMTickCount(TVMTickRef tickref){

};

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    
};
TVMStatus VMThreadDelete(TVMThreadID thread){

};
TVMStatus VMThreadActivate(TVMThreadID thread){

};
TVMStatus VMThreadTerminate(TVMThreadID thread){

};
TVMStatus VMThreadID(TVMThreadIDRef threadref){

};
TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){

};
TVMStatus VMThreadSleep(TVMTick tick){

};

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){

};
TVMStatus VMFileClose(int filedescriptor){

};      
TVMStatus VMFileRead(int filedescriptor, void *data, int *length){

};
TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
    MachineFileWrite(filedescriptor, data, *length, EmptyCallback, NULL);
};
TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){

};
TVMStatus VMFilePrint(int filedescriptor, const char *format, ...){

};

#define VMPrint(format, ...)        VMFilePrint ( 1,  format, ##__VA_ARGS__)
#define VMPrintError(format, ...)   VMFilePrint ( 2,  format, ##__VA_ARGS__)

/*****************************
 * Our definitions are here  *
 *        Functions          *
 * **************************/
int globaltick = 0;
void AlarmCallback(void *calldata){
    //Call the scheduler here
};
void MachineCallback(void *calldata, int result){

};
              

/*****************************
 *         Data Structs      *
 * **************************/

// Thread Control Block
struct TCB{
    // Needed for thread create
    TVMThreadEntry DEntry;
    void * DParam;
    TVMMemorySize DMemsize;
    TVMThreadPriority DPrio;
    TVMThreadIDRef DTID;

    // What will get set later/ don't care about at the time
    int DTicks;
    int DFd;
    TVMThreadState DSTate;
    // Constructor
    TCB(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);
}

TCB::TCB(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    DEntry = entry;
    DParam = param;
    DMemsize = memsize;

};

struct TCBList{
    std::vector<TCB*> DTList;
}

/*****************************
 *         Callback Fns      *
 * **************************/

void EmptyCallback(void *calldata, int result){
    
}