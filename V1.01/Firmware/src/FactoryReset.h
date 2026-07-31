#pragma once

// Returns true when left + middle + right are held continuously during boot.
bool factoryResetRequestedAtBoot();

// Clears all resettable persistent runtime data and reboots.
// The application firmware, SoftDevice and UF2 bootloader are preserved.
[[noreturn]] void factoryResetExecute();
