#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

static int num;

static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_codec_ctx = NULL, *audio_codec_ctx;
static AVStream *video_stream = NULL, *audio_stream = NULL;

static const char *src_filename = NULL;
static const char *video_dst_filename = NULL;
static const char *audio_dst_filename = NULL;
static FILE *video_dst_file = NULL;
static FILE *audio_dst_file = NULL;

static int width, height;
static enum AVPixelFormat pix_fmt;
static uint8_t *video_dst_data[4] = {NULL};
static int video_dst_linesize[4];
static int video_dst_bufsize;

static int video_stream_idx = -1, audio_stream_idx = -1;

static AVFrame *frame = NULL;
static AVPacket pkt;
static int video_frame_count = 0;
static int audio_frame_count = 0;

static int decode_packet(int *got_frame, int cached)
{
    int ret = 0;
    int decoded = pkt.size;
    *got_frame = 0;

    if(pkt.stream_index == video_stream_idx){
        //decode video frame
        ret = avcodec_decode_video2(video_codec_ctx, frame, got_frame, &pkt);
        if(ret < 0){
            fprintf(stderr, "Error decoding video frame (%s) \n",
                    av_err2str(ret));
            return ret;
        }

        printf("num %d got_frame %d\n", num++, *got_frame);
        if(*got_frame){
            av_image_copy(video_dst_data, video_dst_linesize,
                          (const uint8_t **)(frame->data), frame->linesize,
                          pix_fmt, width, height);

            //write to raw video file
            fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
        }
    }else if(pkt.stream_index == video_stream_idx){
        //decode audio frame
        ret = avcodec_decode_audio4(audio_codec_ctx, frame, got_frame, &pkt);
        if(ret < 0){
            fprintf(stderr, "Error decoding audio frame (%s)\n", av_err2str(ret));
            return ret;
        }

        if(*got_frame){
            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(frame->format);
            fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
        }
    }

    return FFMIN(ret, pkt.size);
}

static int open_codec_context(int *stream_idx, 
                              AVFormatContext *fmt_ctx,
                              enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *pStream;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *codec;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if(ret < 0){
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
    }else{
        stream_index = ret;
        pStream = fmt_ctx->streams[stream_index];

        //find decoder for the stream
        codec_ctx = pStream->codec;
        codec = avcodec_find_decoder(codec_ctx->codec_id);
        if(!codec){
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        //open the decoder
        if((ret = avcodec_open2(codec_ctx, codec, NULL))< 0){
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
    }
    *stream_idx = stream_index;
}

int main(int argc, char **argv)
{
    int ret;

    src_filename = argv[1];
    video_dst_filename = argv[2];
    audio_dst_filename = argv[3];

    video_dst_file = fopen(video_dst_filename, "wb+");
    if(!video_dst_file){
        fprintf(stderr, "Coulde not open video dst file\n");
        exit(1);
    }

    audio_dst_file = fopen(audio_dst_filename, "wb+");
    if(!audio_dst_file){
        fprintf(stderr, "Coulde not open audio dst file\n");
        exit(1);
    }

    av_register_all();

    //open input file, and allocate format context
    if(avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0){
        fprintf(stderr, "Could not open source file %s\n", src_filename);   
        exit(1);
    }

    //retrive stream information
    if(avformat_find_stream_info(fmt_ctx, NULL) < 0){
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    if(open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0){
        video_stream    = fmt_ctx->streams[video_stream_idx];
        video_codec_ctx = video_stream->codec;

        //allocate image where the decoded image will be put
        width   = video_codec_ctx->width;
        height  = video_codec_ctx->height;
        pix_fmt = video_codec_ctx->pix_fmt;
        ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             width, height, pix_fmt, 1);
        if(ret < 0){
            fprintf(stderr, "Could not allocate raw video buffer\n");
            exit(1);
        }
        video_dst_bufsize = ret;
    }

    if(open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0){
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_codec_ctx = audio_stream->codec;
    }

    //dump input information to stderr
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    //allocate frame 
    frame = av_frame_alloc();
    if(!frame){
        fprintf(stderr, "Could not allocate frame\n");
        exit(1);
    }

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    //read frames from the file
    int got_frame;
    while(av_read_frame(fmt_ctx, &pkt) >= 0){
        AVPacket orig_pkt = pkt;

        do{
            ret = decode_packet(&got_frame, 0);
            if(ret < 0)
                break;
            pkt.data += ret;
            pkt.size -= ret;
        }while(pkt.size > 0);
        av_free_packet(&orig_pkt);
    }

    avcodec_close(video_codec_ctx);
    avcodec_close(audio_codec_ctx);
    avformat_close_input(&fmt_ctx);
    if(video_dst_file){
        fclose(video_dst_file);
    }

    if(audio_dst_file){
        fclose(audio_dst_file);
    }

    av_frame_free(&frame);
    av_free(video_dst_data[0]);

    return ret < 0;
}
