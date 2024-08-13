#pragma once

#include <kernel/ACPI/AML/Bytes.h>
#include <kernel/ACPI/AML/Integer.h>
#include <kernel/ACPI/AML/Node.h>
#include <kernel/ACPI/AML/ParseContext.h>
#include <kernel/ACPI/AML/Pkg.h>
#include <kernel/ACPI/AML/String.h>

namespace Kernel::ACPI::AML
{

	struct Buffer : public AML::Node
	{
		BAN::Vector<uint8_t> buffer;

		Buffer()
			: AML::Node(Node::Type::Buffer)
		{}

		BAN::Optional<bool> logical_compare(BAN::RefPtr<AML::Node> node, AML::Byte binaryop)
		{
			auto rhs = node ? node->as_buffer() : BAN::RefPtr<AML::Buffer>();
			if (!rhs)
			{
				AML_ERROR("Buffer logical compare RHS is not buffer");
				return {};
			}

			(void)binaryop;
			AML_TODO("Logical compare buffer");
			return {};
		}

		BAN::RefPtr<AML::Buffer> as_buffer() override { return this; }

		BAN::RefPtr<AML::Integer> as_integer() override
		{
			uint64_t value = 0;
			for (size_t i = 0; i < BAN::Math::min<size_t>(buffer.size(), 8); i++)
				value |= static_cast<uint64_t>(buffer[i]) << (8 * i);
			return MUST(BAN::RefPtr<Integer>::create(value));
		}

		BAN::RefPtr<AML::Node> evaluate() override
		{
			return this;
		}

		static ParseResult parse(AML::ParseContext& context)
		{
			ASSERT(context.aml_data.size() >= 1);
			ASSERT(static_cast<Byte>(context.aml_data[0]) == Byte::BufferOp);
			context.aml_data = context.aml_data.slice(1);

			auto buffer_pkg = AML::parse_pkg(context.aml_data);
			if (!buffer_pkg.has_value())
				return ParseResult::Failure;

			auto buffer_context = context;
			buffer_context.aml_data = buffer_pkg.value();

			auto buffer_size_object = AML::parse_object(buffer_context);
			if (!buffer_size_object.success())
				return ParseResult::Failure;

			auto buffer_size = buffer_size_object.node()->as_integer();
			if (!buffer_size)
				return ParseResult::Failure;

			uint32_t actual_buffer_size = BAN::Math::max<uint32_t>(buffer_size->value, buffer_context.aml_data.size());

			auto buffer = MUST(BAN::RefPtr<Buffer>::create());
			MUST(buffer->buffer.resize(actual_buffer_size, 0));
			for (uint32_t i = 0; i < buffer_context.aml_data.size(); i++)
					buffer->buffer[i] = buffer_context.aml_data[i];

#if AML_DEBUG_LEVEL >= 2
			buffer->debug_print(0);
			AML_DEBUG_PRINTLN("");
#endif

			return ParseResult(buffer);
		}

		virtual void debug_print(int indent) const override
		{
			AML_DEBUG_PRINT_INDENT(indent);
			AML_DEBUG_PRINT("Buffer ({} bytes)", buffer.size());
		}
	};

	struct BufferField : AML::NamedObject
	{
		BAN::RefPtr<AML::Node> buffer;
		size_t field_bit_offset;
		size_t field_bit_size;

		template<typename T> requires BAN::is_same_v<T, AML::Buffer> || BAN::is_same_v<T, AML::String>
		BufferField(AML::NameSeg name, BAN::RefPtr<T> buffer, size_t field_bit_offset, size_t field_bit_size)
			: AML::NamedObject(Node::Type::BufferField, name)
			, buffer(buffer)
			, field_bit_offset(field_bit_offset)
			, field_bit_size(field_bit_size)
		{}

		BAN::RefPtr<AML::Integer> as_integer() override
		{
			ASSERT(buffer);
			ASSERT(buffer->type == AML::Node::Type::Buffer || buffer->type == AML::Node::Type::String);

			const auto& buffer = (this->buffer->type == AML::Node::Type::Buffer)
				? static_cast<AML::Buffer*>(this->buffer.ptr())->buffer
				: static_cast<AML::String*>(this->buffer.ptr())->string;

			uint64_t value = 0;

			// TODO: optimize for whole byte accesses
			for (size_t i = 0; i < BAN::Math::min<size_t>(field_bit_size, 64); i++)
			{
				const size_t bit = field_bit_offset + i;
				value |= static_cast<uint64_t>((buffer[bit / 8] >> (bit % 8)) & 1) << i;
			}

			return MUST(BAN::RefPtr<Integer>::create(value));
		}

		BAN::RefPtr<AML::Node> evaluate() override
		{
			ASSERT(buffer);
			ASSERT(buffer->type == AML::Node::Type::Buffer || buffer->type == AML::Node::Type::String);
			const auto& buffer = (this->buffer->type == AML::Node::Type::Buffer)
				? static_cast<AML::Buffer*>(this->buffer.ptr())->buffer
				: static_cast<AML::String*>(this->buffer.ptr())->string;
			ASSERT(field_bit_offset + field_bit_size <= buffer.size() * 8);

			uint64_t value = 0;

			// TODO: optimize for whole byte accesses
			for (size_t i = 0; i < field_bit_size; i++)
			{
				const size_t bit = field_bit_offset + i;
				value |= static_cast<uint64_t>((buffer[bit / 8] >> (bit % 8)) & 1) << i;
			}

			return MUST(BAN::RefPtr<AML::Integer>::create(value));
		}

		bool store(BAN::RefPtr<AML::Node> node) override
		{
			ASSERT(buffer);
			ASSERT(buffer->type == AML::Node::Type::Buffer || buffer->type == AML::Node::Type::String);
			auto& buffer = (this->buffer->type == AML::Node::Type::Buffer)
				? static_cast<AML::Buffer*>(this->buffer.ptr())->buffer
				: static_cast<AML::String*>(this->buffer.ptr())->string;
			ASSERT(field_bit_offset + field_bit_size <= buffer.size() * 8);

			auto value = node->as_integer();
			if (!value)
				return false;

			// TODO: optimize for whole byte accesses
			for (size_t i = 0; i < field_bit_size; i++)
			{
				const size_t bit = field_bit_offset + 1;
				buffer[bit / 8] &= ~(1 << (bit % 8));
				buffer[bit / 8] |= ((value->value >> i) & 1) << (bit % 8);
			}

			return true;
		}

		static ParseResult parse(AML::ParseContext& context)
		{
			ASSERT(context.aml_data.size() >= 1);

			size_t field_bit_size = 0;
			switch (static_cast<AML::Byte>(context.aml_data[0]))
			{
				case AML::Byte::CreateBitFieldOp:
					field_bit_size = 1;
					break;
				case AML::Byte::CreateByteFieldOp:
					field_bit_size = 8;
					break;
				case AML::Byte::CreateWordFieldOp:
					field_bit_size = 16;
					break;
				case AML::Byte::CreateDWordFieldOp:
					field_bit_size = 32;
					break;
				case AML::Byte::CreateQWordFieldOp:
					field_bit_size = 64;
					break;
				case AML::Byte::ExtOpPrefix:
					ASSERT(context.aml_data.size() >= 2);
					ASSERT(static_cast<AML::ExtOp>(context.aml_data[1]) == AML::ExtOp::CreateFieldOp);
					break;
				default:
					ASSERT_NOT_REACHED();
			}
			context.aml_data = context.aml_data.slice(1 + (static_cast<AML::Byte>(context.aml_data[0]) == AML::Byte::ExtOpPrefix));

			auto buffer_result = AML::parse_object(context);
			if (!buffer_result.success())
				return ParseResult::Failure;
			auto buffer_node = buffer_result.node() ? buffer_result.node()->as_buffer() : BAN::RefPtr<AML::Buffer>();
			if (!buffer_node || buffer_node->type != Node::Type::Buffer)
			{
				AML_ERROR("Buffer source does not evaluate to a Buffer");
				return ParseResult::Failure;
			}
			auto buffer = BAN::RefPtr<AML::Buffer>(static_cast<AML::Buffer*>(buffer_node.ptr()));

			auto index_result = AML::parse_object(context);
			if (!index_result.success())
				return ParseResult::Failure;
			auto index = index_result.node() ? index_result.node()->as_integer() : BAN::RefPtr<AML::Integer>();
			if (!index)
			{
				AML_ERROR("Failed to parse index for BufferField");
				return ParseResult::Failure;
			}
			size_t field_bit_offset = index->value;
			if (field_bit_size != 1)
				field_bit_offset *= 8;

			if (field_bit_size == 0)
			{
				auto bit_count_result = AML::parse_object(context);
				if (!index_result.success())
					return ParseResult::Failure;
				auto bit_count = bit_count_result.node() ? bit_count_result.node()->as_integer() : BAN::RefPtr<AML::Integer>();
				if (!bit_count)
				{
					AML_ERROR("Failed to parse bit count for BufferField");
					return ParseResult::Failure;
				}
				field_bit_size = bit_count->value;
			}

			auto field_name = AML::NameString::parse(context.aml_data);
			if (!field_name.has_value())
				return ParseResult::Failure;
			if (field_name->path.empty())
			{
				AML_ERROR("Empty field name for BufferField");
				return ParseResult::Failure;
			}

			auto field = MUST(BAN::RefPtr<BufferField>::create(field_name->path.back(), buffer, field_bit_offset, field_bit_size));
			if (!Namespace::root_namespace()->add_named_object(context, field_name.value(), field))
				return ParseResult::Failure;

#if AML_DEBUG_LEVEL >= 2
			field->debug_print(0);
			AML_DEBUG_PRINTLN("");
#endif

			return ParseResult::Success;
		}

		virtual void debug_print(int indent) const override
		{
			AML_DEBUG_PRINT_INDENT(indent);
			AML_DEBUG_PRINT("BufferField {} at bit offset {} ({} bits) to { ", name, field_bit_offset, field_bit_size);
			buffer->debug_print(0);
			AML_DEBUG_PRINT(" }");
		}

	};

}
