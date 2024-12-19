#include <iostream>
#include <ffmpeg/avformat.h>
#include <ffmpeg/avcodec.h>
#include <ffmpeg/swresample.h>

int main() {
    // Initialize FFmpeg
    av_register_all();

    // Open the media file
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, "sample.mp3", nullptr, nullptr) != 0) {
        std::cerr << "Failed to open file." << std::endl;
        return -1;
    }

    // Find the audio stream
    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1) {
        std::cerr << "Audio stream not found." << std::endl;
        return -1;
    }

    // Decode and process audio here...

    // Clean up
    avformat_close_input(&formatContext);

    return 0;
}

libvlc_instance_t* instance = libvlc_new(0, nullptr);
libvlc_media_player_t* player = libvlc_media_player_new(instance);
libvlc_media_t* media = libvlc_media_new_path(instance, "media_file.mp4");
libvlc_media_player_set_media(player, media);
libvlc_media_player_play(player);
