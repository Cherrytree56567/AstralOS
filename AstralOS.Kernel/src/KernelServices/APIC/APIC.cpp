#include "APIC.h"

/*
 * Code from OSDev
 * https://wiki.osdev.org/APIC
 * Returns a 'true' value if the CPU supports APIC
 * and if the local APIC hasn't been disabled in MSRs
 * Note: This requires CPUID to be supported.
 */
bool APIC::CheckAPIC() {
   uint32_t eax, edx;
   cpuid(1, &eax, &edx);
   return edx & CPUID_FEAT_EDX_APIC;
}

/* 
 * Set the physical address for local APIC registers 
 */
void APIC::SetAPICBase(uintptr_t apic) {
   uint32_t edx = 0;
   uint32_t eax = (apic & 0xfffff0000) | IA32_APIC_BASE_MSR_ENABLE;

#ifdef __PHYSICAL_MEMORY_EXTENSION__
   edx = (apic >> 32) & 0x0f;
#endif

   WriteMSR(IA32_APIC_BASE_MSR, eax, edx);
}

/*
 * Get the physical address of the APIC registers page
 * make sure you map it to virtual memory ;)
 */
uintptr_t APIC::GetAPICBase() {
   uint32_t eax, edx;
   ReadMSR(IA32_APIC_BASE_MSR, &eax, &edx);

#ifdef __PHYSICAL_MEMORY_EXTENSION__
   return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
#else
   return (eax & 0xfffff000);
#endif
}

/*
 * Section 11.4.1 of 3rd volume of Intel SDM recommends 
 * mapping the base address page as strong uncacheable 
 * for correct APIC operation. 
 * 
 * Hardware enable the Local APIC if it wasn't enabled
 * 
 * Set the Spurious Interrupt Vector Register bit 8 to start receiving interrupts
 */
void APIC::EnableAPIC() {
    SetAPICBase(GetAPICBase());
    uint32_t val = ReadAPIC(static_cast<APICRegs>(APICRegs::SIV));
    val |= 0x100;
    WriteAPIC(static_cast<APICRegs>(APICRegs::SIV), val);
}

bool APIC::IsInterruptPending() {
    for (int i = 0; i < 8; i++) {
        uint32_t irr = ReadAPIC(static_cast<APICRegs>(static_cast<uintptr_t>(APICRegs::IRR0) + i * 0x10));
        if (irr != 0) return true;
    }
    return false;
}

void APIC::ReadMSR(uint32_t msr, uint32_t* eax, uint32_t* edx) {
    __asm__ volatile ("rdmsr"
                      : "=a"(*eax), "=d"(*edx)
                      : "c"(msr));
}

void APIC::WriteMSR(uint32_t msr, uint32_t eax, uint32_t edx) {
    __asm__ volatile ("wrmsr"
                      :
                      : "c"(msr), "a"(eax), "d"(edx));
}

uint32_t APIC::ReadAPIC(APICRegs reg) {
    volatile uint32_t* addr = reinterpret_cast<volatile uint32_t*>(GetAPICBase() + static_cast<uintptr_t>(reg));
    return *addr;
}

void APIC::WriteAPIC(APICRegs reg, uint32_t val) {
    volatile uint32_t* addr = reinterpret_cast<volatile uint32_t*>(GetAPICBase() + static_cast<uintptr_t>(reg));
    *addr = val;
}

uint8_t LocalVectorTable::GetVector() {
    return value & 0xFF;
}

uint8_t LocalVectorTable::GetDeliveryMode() {
    return (value >> 8) & 0x7;
}

bool LocalVectorTable::GetInterruptPending() {
    return (value >> 12) & 0x1;
}

bool LocalVectorTable::GetPolarity() {
    return (value >> 13) & 0x1;
}

bool LocalVectorTable::GetRemoteIRR() {
    return (value >> 14) & 0x1;
}

bool LocalVectorTable::GetTriggerMode() {
    return (value >> 15) & 0x1;
}

bool LocalVectorTable::GetMasked() {
    return (value >> 16) & 0x1;
}

void LocalVectorTable::SetVector(uint8_t val) {
    value = (value & ~0xFF) | (val & 0xFF);
}

void LocalVectorTable::SetDeliveryMode(uint8_t val) {
    value = (value & ~(0x7 << 8)) | ((val & 0x7) << 8);
}

void LocalVectorTable::SetInterruptPending(bool flag) {
    value = (value & ~(1 << 12)) | (flag << 12);
}

void LocalVectorTable::SetPolarity(bool flag) {
    value = (value & ~(1 << 13)) | (flag << 13);
}

void LocalVectorTable::SetRemoteIRR(bool flag) {
    value = (value & ~(1 << 14)) | (flag << 14);
}

void LocalVectorTable::SetTriggerMode(bool flag) {
    value = (value & ~(1 << 15)) | (flag << 15);
}

void LocalVectorTable::SetMasked(bool flag) {
    value = (value & ~(1 << 16)) | (flag << 16);
}

uint8_t InterruptCommandRegister::GetVector() {
    return value & 0xFF;
}

uint8_t InterruptCommandRegister::GetDeliveryMode() {
    return (value >> 8) & 0x7;
}

bool InterruptCommandRegister::GetDestinationMode() {
    return (value >> 11) & 0x1;
}

bool InterruptCommandRegister::GetDeliveryStatus() {
    return (value >> 12) & 0x1;
}

bool InterruptCommandRegister::GetInitClear() {
    return (value >> 14) & 0x1;
}

bool InterruptCommandRegister::GetInitSet() {
    return (value >> 15) & 0x1;
}

uint8_t InterruptCommandRegister::GetDestinationType() {
    return (value >> 18) & 0x3; 
}

void InterruptCommandRegister::SetVector(uint8_t val) {
    value = (value & ~0xFF) | (val & 0xFF);
}

void InterruptCommandRegister::SetDeliveryMode(uint8_t val) {
    value = (value & ~(0x7 << 8)) | ((val & 0x7) << 8);
}

void InterruptCommandRegister::SetDestinationMode(bool flag) {
    value = (value & ~(1 << 11)) | (flag << 11);
}

void InterruptCommandRegister::SetDeliveryStatus(bool flag) {
    value = (value & ~(1 << 12)) | (flag << 12);
}

void InterruptCommandRegister::SetInitClear(bool flag) {
    value = (value & ~(1 << 14)) | (flag << 14);
}

void InterruptCommandRegister::SetInitSet(bool flag) {
    value = (value & ~(1 << 15)) | (flag << 15);
}

void InterruptCommandRegister::SetDestinationType(uint8_t destType) {
    value = (value & ~(0x3 << 18)) | ((destType & 0x3) << 18);
}