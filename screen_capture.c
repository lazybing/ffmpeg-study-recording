#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>

int main(int argc, char **argv)
{
    av_register_all();
    avdevice_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVInputFormat   *iformt     = av_find_input_format("video4linux2");
    avformat_open_input(&pFormatCtx, "video=x11grab", iformt, NULL);

}
