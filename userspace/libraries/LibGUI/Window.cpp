#include <LibGUI/Window.h>

#include <BAN/ScopeGuard.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/banan-os.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace LibGUI
{

	struct ReceivePacket
	{
		PacketType type;
		BAN::Vector<uint8_t> data_with_type;
	};

	static BAN::ErrorOr<ReceivePacket> recv_packet(int socket)
	{
		uint32_t packet_size;

		{
			const ssize_t nrecv = recv(socket, &packet_size, sizeof(uint32_t), 0);
			if (nrecv < 0)
				return BAN::Error::from_errno(errno);
			if (nrecv == 0)
				return BAN::Error::from_errno(ECONNRESET);
		}

		if (packet_size < sizeof(uint32_t))
			return BAN::Error::from_literal("invalid packet, does not fit packet id");

		BAN::Vector<uint8_t> packet_data;
		TRY(packet_data.resize(packet_size));

		size_t total_recv = 0;
		while (total_recv < packet_size)
		{
			const ssize_t nrecv = recv(socket, packet_data.data() + total_recv, packet_size - total_recv, 0);
			if (nrecv < 0)
				return BAN::Error::from_errno(errno);
			if (nrecv == 0)
				return BAN::Error::from_errno(ECONNRESET);
			total_recv += nrecv;
		}

		return ReceivePacket {
			*reinterpret_cast<PacketType*>(packet_data.data()),
			packet_data
		};
	}

	Window::~Window()
	{
		cleanup();
	}

	BAN::ErrorOr<BAN::UniqPtr<Window>> Window::create(uint32_t width, uint32_t height, BAN::StringView title, Attributes attributes)
	{
		int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (server_fd == -1)
			return BAN::Error::from_errno(errno);
		BAN::ScopeGuard server_closer([server_fd] { close(server_fd); });

		if (fcntl(server_fd, F_SETFD, fcntl(server_fd, F_GETFD) | FD_CLOEXEC) == -1)
			return BAN::Error::from_errno(errno);

		timespec start_time;
		clock_gettime(CLOCK_MONOTONIC, &start_time);

		for (;;)
		{
			sockaddr_un server_address;
			server_address.sun_family = AF_UNIX;
			strcpy(server_address.sun_path, s_window_server_socket.data());
			if (connect(server_fd, (sockaddr*)&server_address, sizeof(server_address)) == 0)
				break;

			timespec current_time;
			clock_gettime(CLOCK_MONOTONIC, &current_time);
			time_t duration_s = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_nsec >= start_time.tv_nsec);
			if (duration_s > 1)
				return BAN::Error::from_errno(ETIMEDOUT);

			timespec sleep_time;
			sleep_time.tv_sec = 0;
			sleep_time.tv_nsec = 1'000'000;
			nanosleep(&sleep_time, nullptr);
		}

		WindowPacket::WindowCreate create_packet;
		create_packet.width = width;
		create_packet.height = height;
		create_packet.attributes = attributes;
		TRY(create_packet.title.append(title));
		TRY(create_packet.send_serialized(server_fd));

		auto window = TRY(BAN::UniqPtr<Window>::create(server_fd, attributes));

		bool resized = false;
		window->set_resize_window_event_callback([&]() { resized = true; });
		while (!resized)
			window->poll_events();
		window->set_resize_window_event_callback({});

		server_closer.disable();

		return window;
	}

	void Window::invalidate(int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		if (!m_texture.clamp_to_texture(x, y, width, height))
			return;

		for (uint32_t i = 0; i < height; i++)
			memcpy(&m_framebuffer_smo[(y + i) * m_width + x], &m_texture.pixels()[(y + i) * m_width + x], width * sizeof(uint32_t));

		WindowPacket::WindowInvalidate packet;
		packet.x = x;
		packet.y = y;
		packet.width = width;
		packet.height = height;

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);
	}

	void Window::set_mouse_capture(bool captured)
	{
		WindowPacket::WindowSetMouseCapture packet;
		packet.captured = captured;

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);
	}

	void Window::set_fullscreen(bool fullscreen)
	{
		WindowPacket::WindowSetFullscreen packet;
		packet.fullscreen = fullscreen;

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);
	}

	void Window::set_title(BAN::StringView title)
	{
		WindowPacket::WindowSetTitle packet;
		MUST(packet.title.append(title));

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);
	}

	void Window::set_position(int32_t x, int32_t y)
	{
		WindowPacket::WindowSetPosition packet;
		packet.x = x;
		packet.y = y;

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);
	}

	void Window::set_min_size(uint32_t width, uint32_t height)
	{
		WindowPacket::WindowSetMinSize packet;
		packet.width = width;
		packet.height = height;

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);
	}

	void Window::set_attributes(Attributes attributes)
	{
		WindowPacket::WindowSetAttributes packet;
		packet.attributes = attributes;

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);

		m_attributes = attributes;
	}

	void Window::request_resize(uint32_t width, uint32_t height)
	{
		WindowPacket::WindowSetSize packet;
		packet.width = width;
		packet.height = height;

		if (auto ret = packet.send_serialized(m_server_fd); ret.is_error())
			return on_socket_error(__FUNCTION__);
	}

	void Window::on_socket_error(BAN::StringView function)
	{
		if (m_handling_socket_error)
			return;
		m_handling_socket_error = true;

		dprintln("Socket error while running Window::{}", function);

		if (!m_socket_error_callback)
			exit(1);

		m_socket_error_callback();
		cleanup();
	}

	void Window::cleanup()
	{
		munmap(m_framebuffer_smo, m_width * m_height * 4);
		close(m_server_fd);
	}

	BAN::ErrorOr<void> Window::handle_resize_event(const EventPacket::ResizeWindowEvent& event)
	{
		if (m_framebuffer_smo)
			munmap(m_framebuffer_smo, m_width * m_height * 4);
		m_framebuffer_smo = nullptr;

		TRY(m_texture.resize(event.width, event.height));

		void* framebuffer_addr = smo_map(event.smo_key);
		if (framebuffer_addr == nullptr)
			return BAN::Error::from_errno(errno);

		m_framebuffer_smo = static_cast<uint32_t*>(framebuffer_addr);
		m_width = event.width;
		m_height = event.height;

		invalidate();

		return {};
	}

	void Window::wait_events()
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_server_fd, &fds);
		select(m_server_fd + 1, &fds, nullptr, nullptr, nullptr);
	}

	void Window::poll_events()
	{
#define TRY_OR_BREAK(...) ({ auto&& e = (__VA_ARGS__); if (e.is_error()) break; e.release_value(); })
		for (;;)
		{
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(m_server_fd, &fds);
			timeval timeout { .tv_sec = 0, .tv_usec = 0 };
			select(m_server_fd + 1, &fds, nullptr, nullptr, &timeout);

			if (!FD_ISSET(m_server_fd, &fds))
				break;

			auto packet_or_error = recv_packet(m_server_fd);
			if (packet_or_error.is_error())
				return on_socket_error(__FUNCTION__);

			const auto [packet_type, packet_data] = packet_or_error.release_value();
			switch (packet_type)
			{
				case PacketType::DestroyWindowEvent:
					exit(1);
				case PacketType::CloseWindowEvent:
					if (m_close_window_event_callback)
						m_close_window_event_callback();
					else
						exit(0);
					break;
				case PacketType::ResizeWindowEvent:
				{
					MUST(handle_resize_event(TRY_OR_BREAK(EventPacket::ResizeWindowEvent::deserialize(packet_data.span()))));
					if (m_resize_window_event_callback)
						m_resize_window_event_callback();
					break;
				}
				case PacketType::KeyEvent:
					if (m_key_event_callback)
						m_key_event_callback(TRY_OR_BREAK(EventPacket::KeyEvent::deserialize(packet_data.span())).event);
					break;
				case PacketType::MouseButtonEvent:
					if (m_mouse_button_event_callback)
						m_mouse_button_event_callback(TRY_OR_BREAK(EventPacket::MouseButtonEvent::deserialize(packet_data.span())).event);
					break;
				case PacketType::MouseMoveEvent:
					if (m_mouse_move_event_callback)
						m_mouse_move_event_callback(TRY_OR_BREAK(EventPacket::MouseMoveEvent::deserialize(packet_data.span())).event);
					break;
				case PacketType::MouseScrollEvent:
					if (m_mouse_scroll_event_callback)
						m_mouse_scroll_event_callback(TRY_OR_BREAK(EventPacket::MouseScrollEvent::deserialize(packet_data.span())).event);
					break;
				default:
					break;
			}
		}
#undef TRY_OR_BREAK
	}

}
