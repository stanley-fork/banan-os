#pragma once

#include <BAN/Array.h>
#include <kernel/Device/Device.h>
#include <kernel/Lock/SpinLock.h>
#include <kernel/Terminal/TerminalDriver.h>
#include <kernel/Terminal/termios.h>
#include <kernel/Semaphore.h>
#include <LibInput/KeyEvent.h>

namespace Kernel
{

	class TTY : public CharacterDevice
	{
	public:
		void set_termios(const termios& termios) { m_termios = termios; }
		termios get_termios() const { return m_termios; }
		virtual void set_font(const LibFont::Font&) {};

		void set_foreground_pgrp(pid_t pgrp) { m_foreground_pgrp = pgrp; }
		pid_t foreground_pgrp() const { return m_foreground_pgrp; }

		BAN::ErrorOr<void> tty_ctrl(int command, int flags);

		// for kprint
		static void putchar_current(uint8_t ch);
		static bool is_initialized();
		static BAN::RefPtr<TTY> current();
		void set_as_current();

		static void initialize_devices();
		void on_key_event(LibInput::KeyEvent);
		void handle_input_byte(uint8_t);

		virtual bool is_tty() const override { return true; }

		virtual uint32_t height() const = 0;
		virtual uint32_t width() const = 0;
		void putchar(uint8_t ch);

		virtual void clear() = 0;

		virtual BAN::ErrorOr<void> chmod_impl(mode_t) override;
		virtual BAN::ErrorOr<void> chown_impl(uid_t, gid_t) override;

		virtual BAN::ErrorOr<long> ioctl_impl(int, void*) override;

		virtual bool can_read_impl() const override { return m_output.flush; }
		virtual bool can_write_impl() const override { return true; }
		virtual bool has_error_impl() const override { return false; }

	protected:
		TTY(mode_t mode, uid_t uid, gid_t gid)
			: CharacterDevice(mode, uid, gid)
		{ }

		virtual void putchar_impl(uint8_t ch) = 0;
		virtual BAN::ErrorOr<size_t> read_impl(off_t, BAN::ByteSpan) override;
		virtual BAN::ErrorOr<size_t> write_impl(off_t, BAN::ConstByteSpan) override;

	private:
		void do_backspace();

	protected:
		TerminalDriver::Color m_foreground { TerminalColor::BRIGHT_WHITE };
		TerminalDriver::Color m_background { TerminalColor::BLACK };
		termios m_termios;

	private:
		pid_t m_foreground_pgrp { 0 };

		struct tty_ctrl_t
		{
			bool draw_graphics { true };
			bool receive_input { true };
			Semaphore semaphore;
		};
		tty_ctrl_t m_tty_ctrl;

		struct Buffer
		{
			BAN::Array<uint8_t, 1024> buffer;
			size_t bytes { 0 };
			bool flush { false };
			Semaphore semaphore;
		};
		Buffer m_output;

		RecursiveSpinLock m_write_lock;
	};

}
