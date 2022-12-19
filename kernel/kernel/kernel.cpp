#include <kernel/APIC.h>
#include <kernel/GDT.h>
#include <kernel/IDT.h>
#include <kernel/IO.h>
#include <kernel/Keyboard.h>
#include <kernel/kmalloc.h>
#include <kernel/kprint.h>
#include <kernel/multiboot.h>
#include <kernel/panic.h>
#include <kernel/PIC.h>
#include <kernel/PIT.h>
#include <kernel/RTC.h>
#include <kernel/Serial.h>
#include <kernel/Shell.h>
#include <kernel/tty.h>
#include <kernel/VESA.h>

#define DISABLE_INTERRUPTS() asm volatile("cli")
#define ENABLE_INTERRUPTS() asm volatile("sti")

multiboot_info_t* s_multiboot_info;

extern "C"
void kernel_main(multiboot_info_t* mbi, uint32_t magic)
{
	DISABLE_INTERRUPTS();

	Serial::initialize();
	if (magic != 0x2BADB002)
	{
		dprintln("Invalid multiboot magic number");
		return;
	}

	s_multiboot_info = mbi;

	if (!VESA::Initialize())
	{
		dprintln("Could not initialize VESA");
		return;
	}
	TTY::initialize();

	kmalloc_initialize();

	APIC::Initialize();
	gdt_initialize();
	IDT::initialize();

	PIT::initialize();
	if (!Keyboard::initialize())
		return;

	ENABLE_INTERRUPTS();

	kprintln("Hello from the kernel!");

	auto& shell = Kernel::Shell::Get();

	shell.Run();

	for (;;)
	{
		asm("hlt");
	}
}