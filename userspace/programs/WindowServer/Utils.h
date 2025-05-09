#pragma once

#include <BAN/Math.h>
#include <BAN/Optional.h>

#include <stdint.h>

struct Position
{
	int32_t x;
	int32_t y;
};

struct Rectangle
{
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;

	bool contains(Position position) const
	{
		if (position.x < x || position.x >= x + width)
			return false;
		if (position.y < y || position.y >= y + height)
			return false;
		return true;
	}

	BAN::Optional<Rectangle> get_overlap(Rectangle other) const
	{
		if (height == 0 || width == 0 || other.width == 0 || other.height == 0)
			return {};
		const auto min_x = BAN::Math::max(x, other.x);
		const auto min_y = BAN::Math::max(y, other.y);
		const auto max_x = BAN::Math::min(x + width, other.x + other.width);
		const auto max_y = BAN::Math::min(y + height, other.y + other.height);
		if (min_x >= max_x || min_y >= max_y)
			return {};
		return Rectangle {
			.x = min_x,
			.y = min_y,
			.width = max_x - min_x,
			.height = max_y - min_y,
		};
	}

	Rectangle get_bounding_box(Rectangle other) const
	{
		const auto min_x = BAN::Math::min(x, other.x);
		const auto min_y = BAN::Math::min(y, other.y);
		const auto max_x = BAN::Math::max(x + width, other.x + other.width);
		const auto max_y = BAN::Math::max(y + height, other.y + other.height);
		return Rectangle {
			.x = min_x,
			.y = min_y,
			.width = max_x - min_x,
			.height = max_y - min_y,
		};
	}

	bool operator==(const Rectangle& other) const
	{
		return x == other.x && y == other.y && width == other.width && height == other.height;
	}

};

struct Circle
{
	int32_t x;
	int32_t y;
	int32_t radius;

	bool contains(Position position) const
	{
		int32_t dx = position.x - x;
		int32_t dy = position.y - y;
		return dx * dx + dy * dy <= radius * radius;
	}

};

struct Range
{
	uint32_t start { 0 };
	uint32_t count { 0 };

	bool is_continuous_with(const Range& range) const
	{
		return start <= range.start + range.count && range.start <= start + count;
	}

	uint32_t distance_between(const Range& range) const
	{
		if (is_continuous_with(range))
			return 0;
		if (start < range.start)
			return range.start - (start + count);
		return start - (range.start + range.count);
	}

	void merge_with(const Range& range)
	{
		const uint32_t new_start = BAN::Math::min(start, range.start);
		const uint32_t new_end   = BAN::Math::max(start + count, range.start + range.count);

		start = new_start;
		count = new_end - new_start;
	}
};
