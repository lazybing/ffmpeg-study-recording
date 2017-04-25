/*
 * @file
 * libavcodec API use example
 *
 * @example decoding_encoding.c
 */

#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

/*
 * video decoding example
 */
static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    int i;

    f = fopen(filename, "w");
    fprintf(f, "ps\n%d %d\n%d\n", xsize, ysize, 255);
    for(i = 0; i < ysize; i++)
        fwrite(buf + i *wrap, 1, xsize, f);
    flcose(f);
}

static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
                              AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
    int len, got_frame;
    char buf[1024];

    len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);
    if(len < 0){
        fprintf(stderr, "Error while decoding frame %d\n", *frame_count); 
        return len;
    }
    if(got_frame){
        printf("Saving %s frame %3d\n", last?"last":"", *frame_count); 
        fflush(stdout);

        //the picture is allocated by the decoder, no need to free it
        snprintf(buf, sizeof(buf), outfilename, *frame_count);
        pgm_save(frame->data[0], frame->linesize[0], 
                 frame->width, frame->height, buf);
        (*frame_count)++;
    }

    if(pkt->data){
        pkt->size -= len; 
        pkt->data += len;
    }

    return 0;
}

static void video_decode_example(const char *outfilename, const char *filename)
{
    AVCodec *codec;
    AVCodecContext *c = NULL;
    int frame_count;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;

    av_init_packet(&avpkt);

    //set end of buffer to 0
    //(this ensures that no overreading happens for damaged mpeg streams)
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    printf("Decode video fil %s to %s\n", filename, outfilename);

    //find the mpeg1 video decoder
    codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
    if(!codec){
        fprintf(stderr, "Codec not found\n"); 
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if(!c){
        fprintf(stderr, "Could not allocate video codec context\n"); 
        exit(1);
    }

    if(codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        c->flags |= AV_CODEC_FLAG_TRUNCATED;  //we do not send complete frames

    //For some codecs, such as msmpeg4 and mpeg4, width and height
    //Must be initialized there because this information is not avalable in the bitstream
    
    //open it
    if(avcodec_open2(c, codec, NULL) < 0){
        fprintf(stderr, "Could not open codec\n"); 
        exit(1);
    }

    f = fopen(filename, "rb");
    if(!f){
        fprintf(stderr, "Could not open %s\n", filename); 
        exit(1);
    }

    frame = av_frame_alloc();
    if(!frame){
        fprintf(stderr, "Could not allocate video frame\n"); 
        exit(1);
    }

    frame_count = 0;
    for(;;){
        avpkt.size = fread(inbuf, 1, INBUF_SIZE, f); 
        if(avpkt.size == 0)
            break;

        /*
         * NOTE1:some codecs are stream based(mpegvideo, mepgaudio)
         * and this is the only method to use them because you cannot
         * konw the compressed data size before analysing it.
         *
         * BUT some other codecs(mspeg4, mpeg4) are inherently frame
         * based, so you must call them with all the data for one frame
         * exactly. You must also initialize 'width' and 'height' 
         * before initializeing them
         */

        /*
         * NOTE2: some codecs allow the raw parameters(frame size, sample rate)
         * to be changed at any frame, We handle this, so you should also take care of it.
         */

        //here, we use a stream based decoder(mpeg1video), so we feed decoder and see if it could decode a frame
        avpkt.data = inbuf;
        while(avpkt.size > 0)
            if(decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 0) < 0)
                exit(1);
    }

    //some codecs, such as MPEG, transmit the I and P frame with a
    //latency of one frame. You must do the following to have a chance to get the last frame of the video
    avpkt.data = NULL;
    avpkt.size = 0;
    decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 1);

    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_frame_free(&frame);
    printf("\n");
}


int main(int argc, char **argv)
{
    const char *output_type;

    //register all the codecs
    avcodec_register_all();

    if(argc < 2){
        printf("usage:%s output_type\n"
               "API example program to decode/encode a media stream with libavcodec.\n"
               "This is program generates a synthetic stream and encodes it to a file\n"
               "named test.h264, test.mp2 or test.mpg depending on output type\n"
               "The encoded stream is then decoded and written to a raw data output\n"
               "output type must be chosen between 'h264', 'mp2', 'mpg'\n",
               argv[0]);
        return 1;
    }

    output_type = argv[1];

    if(!strcmp(output_type, "h264")){
        video_encode_example("test.h264", AV_CODEC_ID_H264);
    }else if(!strcmp(output_type, "mp2")){
        audio_encode_example("test.mp2"); 
        audio_decode_example("test.pcm", "test.mp2");
    }else if(!strcmp(output_type, "mpg")){
        video_encode_example("test.mpg", AV_CODEC_ID_MPEG1VIDEO);
        video_decode_example("test%02d.pgm", "test.mpg");
    }else{
        fprintf(stderr, "Invalid output type '%s', choose between 'h264', 'mp2', or 'mpg'\n",
                output_type); 
        return 1;
    }

    return 0;
}

