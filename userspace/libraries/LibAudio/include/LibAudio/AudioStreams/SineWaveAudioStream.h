#pragma once

#include <LibAudio/AudioStream.h>

#include <BAN/Limits.h>

namespace LibAudio
{

	class SineWaveAudioStream : public AudioStream
	{
	public:
		static BAN::ErrorOr<BAN::UniqPtr<AudioStream>> create(uint32_t channels, uint32_t sample_rate, float frequency);

		uint32_t channels() const override { return m_channels; }
		uint32_t sample_rate() const override { return m_sample_rate; }
		uint32_t samples_remaining() const override { return BAN::numeric_limits<uint32_t>::max(); }

		float get_sample() override;

	private:
		SineWaveAudioStream(uint32_t channels, uint32_t sample_rate, float frequency);

	private:
		const uint32_t m_channels;
		const uint32_t m_sample_rate;
		const float    m_frequency;
		const float    m_phase_inc;

		uint32_t m_sample_idx { 0 };
		float    m_sample     { 0 };
		float    m_phase      { 0 };
	};

}
