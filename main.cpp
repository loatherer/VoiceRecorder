extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>  // Added for av_gettime()
}

#include <iostream>
#include <chrono>
#include <thread>

int main() {
    // Initialize FFmpeg
    avdevice_register_all();

    // 1. Configure input (microphone)
    AVFormatContext* input_ctx = nullptr;
    AVDictionary* options = nullptr;
    av_dict_set(&options, "sample_rate", "44100", 0);
    av_dict_set(&options, "channels", "2", 0);
    av_dict_set(&options, "sample_fmt", "s16", 0);
    av_dict_set(&options, "audio_buffer_size", "1024", 0);

    // Open audio device (Windows)
    const char* device_name = "audio=Stereo Mix (Realtek(R) Audio)";
    if (avformat_open_input(&input_ctx, device_name, av_find_input_format("dshow"), &options) < 0) {
        std::cerr << "Failed to open audio device: " << device_name << std::endl;
        return 1;
    }

    // Find audio stream
    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream info" << std::endl;
        return 1;
    }

    // Print device info for debugging
    av_dump_format(input_ctx, 0, device_name, 0);

    int audio_stream = -1;
    for (unsigned i = 0; i < input_ctx->nb_streams; i++) {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = i;
            break;
        }
    }

    if (audio_stream == -1) {
        std::cerr << "No audio stream found" << std::endl;
        return 1;
    }

    // 2. Configure output
    AVFormatContext* output_ctx = nullptr;
    avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, "output.wav");
    if (!output_ctx) {
        std::cerr << "Failed to create output context" << std::endl;
        return 1;
    }

    // 3. Setup audio encoder (PCM for WAV)
    const AVCodec* audio_codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    AVStream* stream = avformat_new_stream(output_ctx, audio_codec);
    AVCodecContext* codec_ctx = avcodec_alloc_context3(audio_codec);

    codec_ctx->sample_rate = 44100;
    AVChannelLayout stereo_layout = AV_CHANNEL_LAYOUT_STEREO;
    av_channel_layout_copy(&codec_ctx->ch_layout, &stereo_layout);
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    codec_ctx->bits_per_raw_sample = 16;

    if (avcodec_open2(codec_ctx, audio_codec, nullptr) < 0) {
        std::cerr << "Failed to open codec" << std::endl;
        return 1;
    }
    avcodec_parameters_from_context(stream->codecpar, codec_ctx);

    // 4. Open output file
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, "output.wav", AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Failed to open output file" << std::endl;
            return 1;
        }
    }

    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cerr << "Failed to write header" << std::endl;
        return 1;
    }

    // 5. Main capture loop
    AVPacket* pkt = av_packet_alloc();
    auto start_time = std::chrono::steady_clock::now();  // Using std::chrono instead

    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(30)) {
        if (av_read_frame(input_ctx, pkt) >= 0 && pkt->stream_index == audio_stream) {
            // Simple passthrough - adjust timestamps
            pkt->pts = av_rescale_q(pkt->pts,
                                  input_ctx->streams[audio_stream]->time_base,
                                  stream->time_base);
            pkt->dts = pkt->pts;
            pkt->duration = av_rescale_q(pkt->duration,
                                        input_ctx->streams[audio_stream]->time_base,
                                        stream->time_base);
            pkt->pos = -1;
            pkt->stream_index = stream->index;

            if (av_interleaved_write_frame(output_ctx, pkt) < 0) {
                std::cerr << "Error writing audio frame" << std::endl;
            }
            av_packet_unref(pkt);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // 6. Cleanup
    av_write_trailer(output_ctx);
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_ctx->pb);
    }

    avformat_close_input(&input_ctx);
    avformat_free_context(output_ctx);
    avcodec_free_context(&codec_ctx);
    av_packet_free(&pkt);

    std::cout << "Audio capture complete - saved to output.wav" << std::endl;
    return 0;
}