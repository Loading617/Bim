extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

av_register_all();
AVFormatContext* formatContext = avformat_alloc_context();
if (avformat_open_input(&formatContext, "media_file.mp4", nullptr, nullptr) != 0) {
    std::cerr << "Failed to open file." << std::endl;
}
