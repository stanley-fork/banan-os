#include <kernel/Device/FramebufferDevice.h>
#include <kernel/Graphics/BGA.h>
#include <kernel/IO.h>
#include <kernel/Terminal/FramebufferTerminal.h>
#include <kernel/Terminal/VirtualTTY.h>

namespace Kernel
{

	enum BGA_IO_PORT : uint16_t
	{
		BGA_IO_PORT_INDEX = 0x1CE,
		BGA_IO_PORT_DATA  = 0x1CF,
	};

	enum BGA_REG : uint16_t
	{
		BGA_REG_ID          = 0,
		BGA_REG_XRES        = 1,
		BGA_REG_YRES        = 2,
		BGA_REG_BPP         = 3,
		BGA_REG_ENABLE      = 4,
		BGA_REG_BANK        = 5,
		BGA_REG_XRES_VIRT  = 6,
		BGA_REG_YRES_VIRT = 7,
		BGA_REG_XOFF    = 8,
		BGA_REG_YOFF    = 9,
	};

	enum BGA_ID : uint16_t
	{
		BGA_ID_0 = 0xB0C0,
		BGA_ID_1 = 0xB0C1,
		BGA_ID_2 = 0xB0C2,
		BGA_ID_3 = 0xB0C3,
		BGA_ID_4 = 0xB0C4,
		BGA_ID_5 = 0xB0C5,
	};

	enum BGA_ENABLE : uint16_t
	{
		BGA_ENABLE_ENABLE     = 0x01,
		BGA_ENABLE_LFB_ENABLE = 0x40,
		BGA_ENABLE_NOCLEARMEM = 0x80,
	};

	BAN::ErrorOr<BAN::RefPtr<BGAController>> BGAController::create(PCI::Device& pci_device)
	{
		auto* bga_controller_ptr = new BGAController(pci_device);
		if (bga_controller_ptr == nullptr)
			return BAN::Error::from_errno(ENOMEM);
		auto bga_controller = BAN::RefPtr<BGAController>::adopt(bga_controller_ptr);
		TRY(bga_controller->initialize());
		return bga_controller;
	}

	BGAController::BGAController(PCI::Device& pci_device)
		: m_pci_device(pci_device)
	{ }

	BAN::ErrorOr<void> BGAController::initialize()
	{
		auto boot_framebuffer = FramebufferDevice::boot_framebuffer();

		m_lfb_bar = TRY(m_pci_device.allocate_bar_region(0));
		if (m_lfb_bar->type() != PCI::BarType::MEM)
		{
			dwarnln("BGA LFB is not memory bar");
			return BAN::Error::from_errno(EINVAL);
		}

		m_fix_info = {
			.xpanstep = 1,
			.ypanstep = 1,
			.pitch = 0,
			.mem_start = static_cast<uint64_t>(m_lfb_bar->paddr()),
			.mem_size  = static_cast<uint32_t>(m_lfb_bar->size()),
		};

		const auto ready_to_use_flags = BGA_ENABLE_LFB_ENABLE | BGA_ENABLE_ENABLE;
		if ((read_reg(BGA_REG_ENABLE) & ready_to_use_flags) == ready_to_use_flags)
		{
			m_var_info = {
				.xres      = read_reg(BGA_REG_XRES),
				.yres      = read_reg(BGA_REG_YRES),
				.xres_virt = read_reg(BGA_REG_XRES_VIRT),
				.yres_virt = read_reg(BGA_REG_XRES_VIRT),
				.xoff      = read_reg(BGA_REG_XOFF),
				.yoff      = read_reg(BGA_REG_YOFF),
				.bpp       = read_reg(BGA_REG_BPP),
			};

			m_fix_info.pitch = m_var_info.xres_virt * m_var_info.bpp / 8;
		}
		else
		{
			const uint32_t xres = boot_framebuffer ? boot_framebuffer->width()  : 1280;
			const uint32_t yres = boot_framebuffer ? boot_framebuffer->height() :  800;
			const uint32_t bpp  = boot_framebuffer ? boot_framebuffer->bpp()    :   32;

			TRY(set_fb_var_info({
				.xres = xres,
				.yres = yres,
				.xres_virt = xres,
				.yres_virt = yres * 2,
				.xoff = 0,
				.yoff = 0,
				.bpp = bpp,
			}));
		}

		if (boot_framebuffer)
		{
			// FIXME: check that this is actually the boot framebuffer
			TRY(boot_framebuffer->set_bga_controller(this));
		}
		else
		{
			auto framebuffer_device = TRY(FramebufferDevice::create(this));
			auto fb_terminal_driver = TRY(FramebufferTerminalDriver::create(framebuffer_device));

			// FIXME: query vtty instead of checking the current tty
			if (auto tty = TTY::current(); tty && tty->is_vtty())
				TRY(static_cast<VirtualTTY*>(tty.ptr())->set_terminal_driver(fb_terminal_driver));
			else
				TRY(VirtualTTY::create(fb_terminal_driver));
		}

		return {};
	}

	BAN::ErrorOr<void> BGAController::set_fb_var_info(const fb_var_info& var_info)
	{
		const auto validate_u16 = [](auto value) -> BAN::ErrorOr<void> {
			if (value > BAN::numeric_limits<uint16_t>::max())
				return BAN::Error::from_errno(EINVAL);
			return {};
		};
		TRY(validate_u16(var_info.xres));
		TRY(validate_u16(var_info.xres));
		TRY(validate_u16(var_info.xres_virt));
		TRY(validate_u16(var_info.yres_virt));
		TRY(validate_u16(var_info.xoff));
		TRY(validate_u16(var_info.yoff));
		TRY(validate_u16(var_info.bpp));

		if (var_info.xres + var_info.xoff > var_info.xres_virt)
			return BAN::Error::from_errno(EINVAL);
		if (var_info.yres + var_info.yoff > var_info.yres_virt)
			return BAN::Error::from_errno(EINVAL);
		if (static_cast<uint64_t>(var_info.xres_virt) * var_info.yres_virt * var_info.bpp / 8 > m_lfb_bar->size())
			return BAN::Error::from_errno(EINVAL);

		SpinLockGuard _(m_lock);

		const bool needs_reconf =
			(m_var_info.xres      != var_info.xres) ||
			(m_var_info.yres      != var_info.yres) ||
			(m_var_info.xres_virt != var_info.xres_virt) ||
			(m_var_info.yres_virt != var_info.yres_virt) ||
			(m_var_info.bpp       != var_info.bpp);

		if (!needs_reconf)
		{
			write_reg(BGA_REG_XOFF, var_info.xoff);
			write_reg(BGA_REG_YOFF, var_info.yoff);
			m_var_info.xoff = var_info.xoff;
			m_var_info.yoff = var_info.yoff;
			return {};
		}

		const auto old_enable = read_reg(BGA_REG_ENABLE);
		write_reg(BGA_REG_ENABLE, 0);

		write_reg(BGA_REG_BPP,       var_info.bpp);
		write_reg(BGA_REG_XRES,      var_info.xres);
		write_reg(BGA_REG_YRES,      var_info.yres);
		write_reg(BGA_REG_XRES_VIRT, var_info.xres_virt);
		write_reg(BGA_REG_YRES_VIRT, var_info.yres_virt);

		const bool valid_config =
			read_reg(BGA_REG_BPP)       == var_info.bpp &&
			read_reg(BGA_REG_XRES)      == var_info.xres &&
			read_reg(BGA_REG_YRES)      == var_info.yres &&
			read_reg(BGA_REG_XRES_VIRT) == var_info.xres_virt;

		if (!valid_config)
		{
			write_reg(BGA_REG_BPP,       m_var_info.bpp);
			write_reg(BGA_REG_XRES,      m_var_info.xres);
			write_reg(BGA_REG_YRES,      m_var_info.yres);
			write_reg(BGA_REG_XRES_VIRT, m_var_info.xres_virt);
			write_reg(BGA_REG_YRES_VIRT, m_var_info.yres_virt);

			write_reg(BGA_REG_ENABLE, BGA_ENABLE_NOCLEARMEM | old_enable);

			return BAN::Error::from_errno(EINVAL);
		}

		write_reg(BGA_REG_ENABLE, BGA_ENABLE_LFB_ENABLE | BGA_ENABLE_ENABLE);

		// NOTE: At least qemu only sets virtual yres on enable and it will be
		//       maximum that fits within vram. Our initial bounds check should
		//       make sure this always succeeds
		ASSERT(read_reg(BGA_REG_YRES_VIRT) >= var_info.yres_virt);

		write_reg(BGA_REG_XOFF, var_info.xoff);
		write_reg(BGA_REG_YOFF, var_info.yoff);

		m_var_info = var_info;
		m_fix_info.pitch = m_var_info.xres_virt * m_var_info.bpp / 8;

		return {};
	}

	void BGAController::write_reg(uint16_t reg, uint16_t value)
	{
		SpinLockGuard _(m_lock);
		IO::outw(BGA_IO_PORT_INDEX, reg);
		IO::outw(BGA_IO_PORT_DATA, value);
	}

	uint16_t BGAController::read_reg(uint16_t reg)
	{
		SpinLockGuard _(m_lock);
		IO::outw(BGA_IO_PORT_INDEX, reg);
		return IO::inw(BGA_IO_PORT_DATA);
	}

	fb_fix_info BGAController::get_fb_fix_info() const
	{
		SpinLockGuard _(m_lock);
		return m_fix_info;
	}

	fb_var_info BGAController::get_fb_var_info() const
	{
		SpinLockGuard _(m_lock);
		return m_var_info;
	}

}
