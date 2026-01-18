# ---- UI / behavior ----
set confirm off
set pagination off
set print pretty on

# ---- Connect to OpenOCD ----
target extended-remote :3333

# ---- MCU state ----
monitor reset halt
load

# ---- Breakpoints ----
break main
break HardFault_Handler

# --- Registers info ---
info all-registers
p/x $pc
p/x $sp

# --- memory ---
#  SRAM
x/16wx 0x20000000

# FLASH
x/16wx 0x08000000

# ---- Run ----
continue

