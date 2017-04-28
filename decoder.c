#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

#define INBUF_SIZE 4096

static FILE *pInput_File  = NULL; 
static FILE *pOutput_File = NULL; 

static char *Input_FileName  = NULL;
static char *Output_FileName = NULL;

static void pgm_save(unsigned char *buf, int wrap, 
                     int xsize, int ysize, char *filename)
{
    FILE *fd;
    int i;
    fd = fopen(filename, "wb+");
    fprintf(fd, "P5\n%d %d\n %d\n", xsize, ysize, 255);

    for(i = 0; i < ysize; i++){
        fwrite(buf + i*wrap, 1, xsize, fd);
    }
    fclose(fd);
}

static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
                              AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
    int len, got_frame;
    char buf[1024];
    int YLen  = 0;
    int CbLen = 0;
    int CrLen = 0;

    len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);
    if(len < 0){
        fprintf(stderr, "Error while decoding frame %d\n", *frame_count);
        return len;
    }

    printf("len %d got_frame %d\n",len, got_frame);

    if(got_frame){
        printf("Saving %s frame %3d\n", last?"last":"", *frame_count);
        fflush(stdout);

        //the picture is allocated by the decoder, no need to free it
        //pgm_save(frame->data[0], frame->linesize[0],
        //        frame->width, frame->height, buf);
        //(*frame_count)++;
        
        YLen  = (frame->height*frame->width)*1.5;
        CbLen = (frame->height>>1)*(frame->width>>1);
        CrLen = (frame->height>>1)*(frame->width>>1);

        fwrite(frame->data[0],1, YLen, pOutput_File);
        frame->data[0] += YLen;
        /*
        fwrite(frame->data[0],1, CbLen, pOutput_File);
        frame->data[0] += CbLen;
        fwrite(frame->data[0],1, CrLen, pOutput_File);
        frame->data[0] += CrLen;
        */
    }

    if(pkt->data){
        pkt->size -= len;
        pkt->data += len;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int len = 0;
    int frame_count = 0;
    AVCodec *codec = NULL;
    AVCodecContext *codecCtx = NULL;
    AVCodecParserContext *pCodecParserCtx = NULL;
    AVFrame *frame;
    AVPacket pkt;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *pDataPtr;
    size_t  uDataSize;
    

    Input_FileName  = argv[1];
    Output_FileName = argv[2];

    pInput_File = fopen(Input_FileName, "rb+");
    if(!pInput_File){
        fprintf(stderr, "Open input file fail\n");
        exit(1);
    }
    
    pOutput_File = fopen(Output_FileName, "wb+");
    if(!pOutput_File){
        fprintf(stderr, "Open output file fail\n");
        exit(1);
    }

    //set end of buffer to 0
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    printf("Decode video file %s to %s\n", Input_FileName, Output_FileName);

    av_register_all();

    av_init_packet(&pkt);

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!codec){
        fprintf(stderr, "cannot find the decoder\n");
        exit(1);
    }

    codecCtx = avcodec_alloc_context3(codec);
    if(!codecCtx){
        fprintf(stderr, "could not allocate video codec context\n"); 
        exit(1);
    }

    if(codec->capabilities & AV_CODEC_CAP_TRUNCATED){
        codecCtx->flags |= AV_CODEC_FLAG_TRUNCATED;
    }

    pCodecParserCtx = av_parser_init(AV_CODEC_ID_H264);
    if(!pCodecParserCtx){
        fprintf(stderr,"Error:alloc parser fail\n");
        exit(1);
    }

    //open the decoder
    if(avcodec_open2(codecCtx, codec, NULL) < 0){
        fprintf(stderr, "Could not open the decoder\n");
        exit(1);
    }

    //open frame structure
    frame = av_frame_alloc();
    if(!frame){
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    frame_count = 0;
    for(;;){
        uDataSize = fread(inbuf, 1, INBUF_SIZE, pInput_File);
        if(uDataSize == 0)
            break;

        pDataPtr = inbuf;
        while(uDataSize > 0){
            //decode the data in the buffer to AVPacket, include a NAL unit data
            len = av_parser_parse2(pCodecParserCtx, codecCtx, &(pkt.data), &(pkt.size), 
                                   pDataPtr, uDataSize,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
            uDataSize -= len;
            pDataPtr  += len;

            if(pkt.size == 0){
                continue;
            }
            printf("Decode frame pts %d pkt.size %d\n", pkt.pts, pkt.size);


            
            if(decode_write_frame(Output_FileName, codecCtx, frame, &frame_count, &pkt, 0) < 0){
                exit(1);
            }
            
        }
    }

    return 0;
}
