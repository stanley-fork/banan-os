#pragma once

#include <BAN/RefPtr.h>
#include <kernel/Lock/SpinLock.h>
#include <kernel/PCI.h>

#include <sys/framebuffer.h>

namespace Kernel
{

	class BGAController : public BAN::RefCounted<BGAController>
	{
	public:
		static BAN::ErrorOr<BAN::RefPtr<BGAController>> create(PCI::Device&);

		fb_fix_info get_fb_fix_info() const;
		fb_var_info get_fb_var_info() const;
		BAN::ErrorOr<void> set_fb_var_info(const fb_var_info&);

	private:
		BGAController(PCI::Device&);
		BAN::ErrorOr<void> initialize();

		void write_reg(uint16_t reg, uint16_t value);
		uint16_t read_reg(uint16_t reg);

	private:
		mutable RecursiveSpinLock m_lock;
		PCI::Device& m_pci_device;
		BAN::UniqPtr<PCI::BarRegion> m_lfb_bar;

		fb_fix_info m_fix_info {};
		fb_var_info m_var_info {};
	};

}
