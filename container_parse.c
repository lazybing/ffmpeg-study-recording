#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define TRUE  1
#define FALSE 0

int ff_parse(char *str)
{
    int result;
    char filename[1024];

    //Register all format an codecs
    av_register_all();

    AVFormatContext *fmt_ctx = avformat_alloc_context();

    result = avformat_open_input(&fmt_ctx, str, NULL, NULL);
    if(result < 0){
        printf("Can't open file\n"); 
        return result;
    }

    result = avformat_find_stream_info(fmt_ctx, NULL);
    if(result < 0){
        printf("Can't get stream info\n"); 
        return result;
    }

    printf("=================================\n");
    printf("parse the stream info:\n");
    printf("=================================\n");
    printf("Container filename     :%s\n",        fmt_ctx->filename);
    printf("Container input format :%s\n",   fmt_ctx->iformat->name);
    printf("Container nb_stream    :%d\n",      fmt_ctx->nb_streams);
    printf("Container duration     :%llu\n",      fmt_ctx->duration);

    int video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_idx >= 0){
        AVStream *video_stream = fmt_ctx->streams[video_stream_idx];
        printf("=================================\n");
        printf("parse Video info:\n");
        printf("=================================\n");
        printf("Video nb_frames      :%lld\n", video_stream->nb_frames);
        printf("Video codec_id       :%d\n", video_stream->codec->codec_id);
        printf("video codec_name     :%s\n", avcodec_get_name(video_stream->codec->codec_id));
        printf("Video width x height :%d x %d\n", video_stream->codec->width, video_stream->codec->height);
        printf("Video pix_fmt        :%d\n", video_stream->codec->pix_fmt);
        printf("Video bitrate        :%lld kb/s\n", video_stream->codec->bit_rate / 100);
        printf("Video avg_frame_rate :%d fps\n", video_stream->avg_frame_rate.num/video_stream->avg_frame_rate.den);
    }

    int audio_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audio_stream_idx >= 0){
        AVStream *audio_stream = fmt_ctx->streams[audio_stream_idx];
        printf("=================================\n");
        printf("parse Audio info:\n");
        printf("=================================\n");
        printf("Audio codec_id       :%d\n", audio_stream->codec->codec_id);
        printf("Audio codec_name     :%s\n", avcodec_get_name(audio_stream->codec->codec_id));
        printf("Audio sample_rate    :%d\n", audio_stream->codec->sample_rate);
        printf("Audio channels       :%d\n", audio_stream->codec->channels);
        printf("Audio sample_fmt     :%d\n", audio_stream->codec->sample_fmt);
        printf("Audio frame_size     :%d\n", audio_stream->codec->frame_size);
        printf("Audio nb_frames      :%lld\n", audio_stream->nb_frames);
        printf("Audio bitrate        :%lld\n", (int64_t)audio_stream->codec->bit_rate / 100);
    }
    
    return TRUE;
}  

int main(int argc, char **argv)
{

    ff_parse(argv[1]);

    return TRUE;
}

