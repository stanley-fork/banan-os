#include <LibAudio/AudioStreams/SineWaveAudioStream.h>

#include <BAN/Math.h>

namespace LibAudio
{

	BAN::ErrorOr<BAN::UniqPtr<AudioStream>> SineWaveAudioStream::create(uint32_t channels, uint32_t sample_rate, float frequency)
	{
		auto* stream_ptr = new SineWaveAudioStream(channels, sample_rate, frequency);
		if (stream_ptr == nullptr)
			return BAN::Error::from_errno(ENOMEM);
		return BAN::UniqPtr<AudioStream>::adopt(stream_ptr);
	}

	SineWaveAudioStream::SineWaveAudioStream(uint32_t channels, uint32_t sample_rate, float frequency)
		: m_channels(channels)
		, m_sample_rate(sample_rate)
		, m_frequency(frequency)
		, m_phase_inc(2.0f * BAN::numbers::pi_v<float> * m_frequency / m_sample_rate)
	{ }

	float SineWaveAudioStream::get_sample()
	{
		if (m_sample_idx++ % m_channels == 0)
		{
			m_phase += m_phase_inc;
			if (m_phase >= 2.0f * BAN::numbers::pi_v<float>)
				m_phase -= 2.0f * BAN::numbers::pi_v<float>;
			m_sample = BAN::Math::sin(m_phase);
		}

		return m_sample;
	}

}
