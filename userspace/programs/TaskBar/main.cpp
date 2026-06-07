#include <LibAudio/Protocol.h>
#include <LibFont/Font.h>
#include <LibGUI/Window.h>

#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

static BAN::ErrorOr<long long> read_integer_from_file(const char* file)
{
	char buffer[128];

	int fd = open(file, O_RDONLY);
	if (fd == -1)
		return BAN::Error::from_errno(errno);
	const ssize_t nread = read(fd, buffer, sizeof(buffer));
	close(fd);

	if (nread < 0)
		return BAN::Error::from_errno(errno);
	if (nread == 0)
		return BAN::Error::from_errno(ENODATA);

	buffer[nread] = '\0';
	return atoll(buffer);
}

static BAN::String get_battery_percentage()
{
	DIR* dirp = opendir("/dev/batteries");
	if (dirp == nullptr)
		return {};

	BAN::String result;
	while (dirent* dirent = readdir(dirp))
	{
		if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
			continue;

		char buffer[PATH_MAX + 32];

		sprintf(buffer, "/dev/batteries/%s/capacity_full", dirent->d_name);
		auto cap_full = read_integer_from_file(buffer);
		if (cap_full.is_error() || cap_full.value() == 0)
			continue;

		sprintf(buffer, "/dev/batteries/%s/capacity_now", dirent->d_name);
		auto cap_now = read_integer_from_file(buffer);
		if (cap_now.is_error())
			continue;

		auto string = BAN::String::formatted("{} {}% | ", dirent->d_name, cap_now.value() * 100 / cap_full.value());
		if (string.is_error())
			continue;

		(void)result.append(string.value());
	}

	closedir(dirp);

	return result;
}

static BAN::String get_cpu_load(bool show_long)
{
	struct LoadStats
	{
		uint64_t idle_ns;
		uint64_t total_ns;
	};

	static BAN::Vector<LoadStats> cpu_stats;

	BAN::Vector<LoadStats> delta_stats;
	(void)delta_stats.reserve(show_long ? cpu_stats.size() : 1);

	for (size_t i = 0;; i++)
	{
		char buffer[PATH_MAX];
		sprintf(buffer, "/proc/cpu/%zu", i);

		FILE* fp = fopen(buffer, "r");
		if (fp == nullptr)
			break;

		LoadStats stats;
		if (fscanf(fp, "%" SCNu64 " %" SCNu64, &stats.idle_ns, &stats.total_ns) == 2)
		{
			if (i >= cpu_stats.size())
				MUST(cpu_stats.resize(i + 1, {}));

			const LoadStats delta {
				.idle_ns  = stats.idle_ns  - cpu_stats[i].idle_ns,
				.total_ns = stats.total_ns - cpu_stats[i].total_ns,
			};
			cpu_stats[i] = stats;

			if (show_long)
				(void)delta_stats.push_back(delta);
			else
			{
				if (delta_stats.empty())
					(void)delta_stats.push_back(delta);
				else
				{
					delta_stats[0].idle_ns  += delta.idle_ns;
					delta_stats[0].total_ns += delta.total_ns;
				}
			}
		}

		fclose(fp);
	}

	BAN::String result;
	(void)result.append("CPU");

	for (size_t i = 0; i < delta_stats.size(); i++)
	{
		const uint64_t idle_10000 = 10'000 * delta_stats[i].idle_ns / delta_stats[i].total_ns;
		const uint64_t load_10000 = 10'000 - idle_10000;

		auto string = BAN::String::formatted(" {}.{2}%", load_10000 / 100, load_10000 % 100);
		if (string.is_error())
			continue;

		(void)result.append(string.release_value());
	}

	(void)result.append(" | "_sv);

	return result;
}

static int open_audio_server_fd()
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
		return -1;

	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, LibAudio::s_audio_server_socket.data());
	if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
		return -1;

	return fd;
}

static BAN::String get_audio_volume()
{
	static int fd = -1;
	if (fd == -1 && (fd = open_audio_server_fd()) == -1)
		return {};

	const LibAudio::Packet request {
		.type = LibAudio::Packet::GetVolume,
		.parameter = {},
	};
	if (send(fd, &request, sizeof(request), 0) != sizeof(request))
	{
		close(fd);
		fd = -1;
		return {};
	}

	uint32_t response;
	if (recv(fd, &response, sizeof(response), 0) != sizeof(response))
	{
		close(fd);
		fd = -1;
		return {};
	}

	return MUST(BAN::String::formatted("vol {}% | ", response / 10));
}

static BAN::ErrorOr<BAN::String> get_task_bar_string(bool show_long)
{
	BAN::String result;

	TRY(result.append(get_cpu_load(show_long)));

	TRY(result.append(get_battery_percentage()));

	TRY(result.append(get_audio_volume()));

	const time_t current_time = time(nullptr);
	TRY(result.append(ctime(&current_time)));
	result.pop_back();

	return result;
}

int main()
{
	constexpr uint32_t padding = 3;
	constexpr uint32_t bg_color = 0xFF202020;
	constexpr uint32_t fg_color = 0xFFFFFFFF;

	signal(SIGUSR1, [](int) {});

	auto font = MUST(LibFont::Font::load("/usr/share/fonts/lat0-16.psfu"_sv));

	auto attributes = LibGUI::Window::default_attributes;
	attributes.title_bar = false;
	attributes.movable = false;
	attributes.focusable = false;
	attributes.alpha_channel = false;
	attributes.rounded_corners = false;

	auto window = MUST(LibGUI::Window::create(0, font.height() + 2 * padding, "TaskBar", attributes));

	window->set_close_window_event_callback([]() {});

	window->set_position(0, 0);

	bool show_long = false;
	window->set_mouse_button_event_callback([&](auto event) {
		if (event.pressed)
			show_long = !show_long;
	});

	window->texture().fill(bg_color);
	window->invalidate();

	bool is_running = true;

	uint32_t old_text_w = 0;
	const auto update_string =
		[&]()
		{
			auto text_or_error = get_task_bar_string(show_long);
			if (text_or_error.is_error())
				return;
			const auto text = text_or_error.release_value();

			const uint32_t text_w = text.size() * font.width();
			const uint32_t text_h = font.height();
			const uint32_t text_x = window->width() - text_w - padding;
			const uint32_t text_y = padding;

			const uint32_t inval_w = BAN::Math::max(text_w, old_text_w);
			const uint32_t inval_x = window->width() - inval_w - padding;

			auto& texture = window->texture();
			texture.fill_rect(inval_x, text_y, inval_w, text_h, bg_color);
			texture.draw_text(text, font, text_x, text_y, fg_color);
			window->invalidate(inval_x, text_y, inval_w, text_h);

			old_text_w = text_w;
		};

	while (is_running)
	{
		window->poll_events();

		update_string();

		constexpr uint64_t ns_per_s = 1'000'000'000;

		timespec current_ts;
		clock_gettime(CLOCK_REALTIME, &current_ts);

		uint64_t current_ns = 0;
		current_ns += current_ts.tv_sec * ns_per_s;
		current_ns += current_ts.tv_nsec;

		uint64_t target_ns = current_ns;
		if (auto rem = target_ns % ns_per_s)
			target_ns += ns_per_s - rem;

		uint64_t sleep_ns = target_ns - current_ns;

		timespec timeout_ts;
		timeout_ts.tv_sec  = sleep_ns / ns_per_s;
		timeout_ts.tv_nsec = sleep_ns % ns_per_s;

		window->wait_events(&timeout_ts);
	}
}
