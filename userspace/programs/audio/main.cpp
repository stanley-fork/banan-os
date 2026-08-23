#include <LibAudio/Audio.h>
#include <LibAudio/AudioStreams/SineWaveAudioStream.h>

#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s FREQ|FILE\n", argv[0]);
		return 1;
	}

	auto audio = [argv]() -> LibAudio::Audio {
		char* endp;
		float freq = strtof(argv[1], &endp);

		if (*endp != '\0')
		{
			auto audio_or_error = LibAudio::Audio::load(argv[1]);
			if (!audio_or_error.is_error())
				return audio_or_error.release_value();
			fprintf(stderr, "failed to load %s: %s\n", argv[1], audio_or_error.error().get_message());
			exit(1);
		}

		auto sine_stream = LibAudio::SineWaveAudioStream::create(2, 48000, freq);
		if (sine_stream.is_error())
		{
			fprintf(stderr, "failed to create sine wave stream: %s\n", sine_stream.error().get_message());
			exit(1);
		}

		auto audio_or_error = LibAudio::Audio::create(sine_stream.release_value());
		if (audio_or_error.is_error())
		{
			fprintf(stderr, "failed to create sine wave stream: %s\n", audio_or_error.error().get_message());
			exit(1);
		}

		return audio_or_error.release_value();
	}();

	if (auto ret = audio.start(); ret.is_error())
	{
		fprintf(stderr, "failed start playing audio: %s\n", ret.error().get_message());
		return 1;
	}

	while (audio.is_playing())
	{
		usleep(10'000);
		audio.update();
	}

	return 0;
}
