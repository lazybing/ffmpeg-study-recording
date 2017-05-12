#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>

int main(int argc, char **argv)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    const char *in_filename, *out_filename;
    int ret, i;
    int videoindex = -1;
    int frame_index = 0;
    int64_t start_time = 0;
    in_filename = "cuc_ieschool.flv";
//    out_filename = "rtmp://localhost/publishlive/livestream";
    out_filename = "rtmp://192.168.71.143/live/livestream";

    //register all codec and format
    av_register_all();
    //network
    avformat_network_init();
    //input
    if((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0){
        printf("Could not open input file\n");
        exit(1);
    }
    if((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0){
        printf("Failed to retrieve input stream information\n");
        exit(1);
    }

    for(i = 0; i < ifmt_ctx->nb_streams; i++){
        if(ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            videoindex = i;
            break;
        }
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    //Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename);
    if(!ofmt_ctx){
        printf("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        exit(1);
    }
    ofmt = ofmt_ctx->oformat;

    for(i = 0; i < ifmt_ctx->nb_streams; i++){
        //depends the input stream create output AVStream 
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if(!out_stream){
            printf("Faile allocate output stream\n");
            ret = AVERROR_UNKNOWN;
            exit(1);
        }

        //copy the setting of AVCodecContext
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if(ret < 0){
            printf("Failed to copy context from input to output stream codec context\n");
            exit(1);
        }
        out_stream->codec->codec_tag = 0;
        if(ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    //Dump output Format
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    //open output URL
    if(!(ofmt->flags & AVFMT_NOFILE)){
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if(ret < 0){
            printf("Could not open output URL '%s'\n", out_filename);
            exit(1);
        }
    }

    //Write file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if(ret < 0){
        printf("Error occurred when opening output URL\n");
        exit(1);
    }

    start_time = av_gettime();
    while(1){
        AVStream *in_stream, *out_stream;
        //get an AVPacket
        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0)
            break;
        //Delay
        if(pkt.stream_index == videoindex){
            AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
            AVRational time_base_q = {1, AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            if(pts_time > now_time)
                av_usleep(pts_time - now_time);
        }
        
        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        //convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX)); 
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX)); 
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base); 
        pkt.pos = -1; 

        //print to screen
        if(pkt.stream_index == videoindex){
            printf("send %8d video frames ot output URL\n", frame_index);
            frame_index++;
        }

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if(ret < 0){
            printf("Error muxing packet\n");
            break;

        av_free_packet(&pkt);
        }
    }

    av_write_trailer(ofmt_ctx);

end:
    avformat_close_input(&ifmt_ctx);
    //close output
    if(ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    if(ret < 0 && ret != AVERROR_EOF){
        printf("Error occurred.\n");
        return -1;
    }

    return 0;
}

