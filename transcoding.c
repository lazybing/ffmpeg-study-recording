/*
 * @file
 * API example for demuxing, decoding, filtering, encoding and muxing
 * @example transcoding.c
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;
typedef struct FilteringContext{
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph   *filter_graph;
}FilteringContext;
static FilteringContext *filter_ctx;

static int open_input_file(const char *filename)
{

}

static int open_output_file(const char *filename)
{

}

static int init_filter(FilteringContext *fctx, AVCodecContext *dec_ctx,
                       AVCodecContext *enc_ctx, const char *filter_spec)
{

}

static int init_filters(void)
{

}

static int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame)
{

}

static int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index)
{

}

static int flush_encoder(unsigned int stream_index)
{

}

int main(int argc, char **argv)
{
    int ret;
    AVPacket packet = {.data = NULL, .size = 0};
    AVFrame *frame = NULL;
    enum AVMediaType type;
    unsigned int stream_index;
    unsigned int i;
    int got_frame;
    int (*dec_fun)(AVCodecContext *, AVFrame *, int *, const AVPacket *);

    if(argc != 3){
        av_log(NULL, AV_LOG_ERROR, "Usage:%s <input file> <output file>\n", argv[0]);
        return 1;
    }

    av_register_all();
    avfilter_register_all();

    if((ret = open_input_file(argv[1])) < 0)
        goto end;
    if((ret = open_output_file(argv[2])) < 0)
        goto end;
    if((ret = init_filters()) < 0)
        goto end;

    //read all packets
    while(1){
        if((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
            break;
        stream_index = packet.stream_index;
        type = ifmt_ctx->streams[packet.stream_index]->codec->codec_type;
        av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n", 
               stream_index);

        if(filter_ctx[stream_index].filter_graph){
            av_log(NULL, AV_LOG_DEBUG, "Going to reencode & filter the frame\n");
            frame = av_frame_alloc();
            if(!frame){
                ret = AVERROR(ENUMEM);
                break;
            }
            av_packet_rescale_ts(&packet,
                                 ifmt_ctx->streams[stream_index]->time_base,
                                 ifmt_ctx->streams[stream_index]->codec->time_base);
            dec_fun = (type == AVMEDIA_TYPE_VIDEO)?avcodec_decode_video2:avcodec_decode_audio4;
            ret = dec_fun(ifmt_ctx->streams[stream_index]->codec, frame, 
                          &got_frame, &packet);

            if(ret < 0){
                av_frame_free(&frame);
                av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
                break;
            }

            if(got_frame){
                frame->pts = av_frame_get_best_effort_timestamp(frame);
                ret = filter_encode_write_frame(frame, stream_index);
                av_frame_free(&frame);
                if(ret < 0)
                    goto end;
            }else{
                av_frame_free(&frame); 
            }
        }else{
            //remux this frame without reencoding 
            av_packet_rescale_ts(&packet, 
                                 ifmt_ctx->streams[stream_index]->time_base,
                                 ofmt_ctx->streams[stream_index]->time_base);
            ret = av_interleaved_write_frame(ofmt_ctx, &packet);
            if(ret < 0)
                goto end;
        }
        av_free_packet(&packet);
    }

    //flush filters and encoders
    for(i = 0; i < ifmt_ctx->nb_stream; i++){
        //flush filter
        if(!filter_ctx[i].filter_graph) 
            continue;
        ret = filter_encode_write_frame(NULL, i);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "Flushing filter failed\n");
            goto end;
        }

        //flush encoder
        ret = flush_encoder(i);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
            goto end;
        }
    }
    av_write_trailer(ofmt_ctx);

end:
    av_free_packet(&packet);
    av_frame_free(&frame);
    for(i = 0; i < ifmt_ctx->nb_streams;i++){
        avcodec_close(ifmt_ctx->streams[i]->codec); 
        if(ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && ofmt_ctx->streams[i]->codec)
            avcodec_close(ifmt_ctx->streams[i]->codec); 
        if(filter_ctx && filter_ctx[i].filter_graph)
            avfilter_graph_free(&filter_ctx[i].filter_graph);
    }
    av_free(filter_ctx);
    avformat_close_input(&ifmt_ctx);
    if(ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    if(ret , 0)
        av_log(NULL, AV_LOG_ERROR, "Error occurred:%s\n", av_err2str(ret));

    return ret ? 1 : 0;
}
