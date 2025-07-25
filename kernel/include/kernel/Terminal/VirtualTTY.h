#pragma once

#include <BAN/Array.h>
#include <kernel/Device/Device.h>
#include <kernel/Terminal/TerminalDriver.h>
#include <kernel/Terminal/TTY.h>
#include <kernel/ThreadBlocker.h>

namespace Kernel
{

	class VirtualTTY : public TTY
	{
	public:
		using Palette = TerminalDriver::Palette;

	public:
		static BAN::ErrorOr<BAN::RefPtr<VirtualTTY>> create(BAN::RefPtr<TerminalDriver>);

		BAN::ErrorOr<void> set_font(LibFont::Font&&) override;

		void clear() override;

	protected:
		BAN::StringView name() const override { return m_name; }
		bool putchar_impl(uint8_t ch) override;
		bool can_write_impl() const override { return true; }
		void after_write() override;

	private:
		VirtualTTY(BAN::RefPtr<TerminalDriver>);

		void reset_ansi();
		void handle_ansi_csi(uint8_t ch);
		void handle_ansi_csi_color(uint8_t ch);
		void putcodepoint(uint32_t codepoint);
		void putchar_at(uint32_t codepoint, uint32_t x, uint32_t y);
		void render_from_buffer(uint32_t x, uint32_t y);
		void scroll_if_needed();

	private:
		enum class State
		{
			Normal,
			WaitingAnsiEscape,
			WaitingAnsiCSI,
			WaitingUTF8,
		};

		struct AnsiState
		{
			static constexpr size_t max_nums = 5;
			int32_t nums[max_nums]	{ -1, -1, -1, -1, -1 };
			size_t index { 0 };
			bool question { false };
		};

		BAN::Optional<TerminalDriver::Color> get_8bit_color();
		BAN::Optional<TerminalDriver::Color> get_24bit_color();

		struct UTF8State
		{
			uint32_t codepoint { 0 };
			uint8_t bytes_missing { 0 };
		};

		struct Cell
		{
			TerminalDriver::Color foreground;
			TerminalDriver::Color background;
			uint32_t codepoint { ' ' };
		};

	private:
		BAN::String m_name;

		BAN::RefPtr<TerminalDriver> m_terminal_driver;

		State m_state { State::Normal };
		AnsiState m_ansi_state { };
		UTF8State m_utf8_state { };

		uint32_t m_width { 0 };
		uint32_t m_height { 0 };

		uint32_t m_last_graphic_char { 0 };
		uint32_t m_saved_row { 0 };
		uint32_t m_saved_column { 0 };

		bool m_cursor_shown { true };
		uint32_t m_row { 0 };
		uint32_t m_column { 0 };
		Cell* m_buffer { nullptr };

		bool m_last_cursor_shown { false };
		uint32_t m_last_cursor_row { static_cast<uint32_t>(-1) };
		uint32_t m_last_cursor_column { static_cast<uint32_t>(-1) };

		const Palette& m_palette;

		TerminalDriver::Color m_foreground;
		TerminalDriver::Color m_background;
		bool m_colors_inverted { false };
	};

}
