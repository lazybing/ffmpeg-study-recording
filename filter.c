#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>

static char *pInput_File_Name  = NULL;
static char *pOutput_File_Name = NULL;

static FILE *pfdInput  = NULL;
static FILE *pfdOutput = NULL;

static int width;
static int height;

static AVFilterContext *buffersink_ctx;
static AVFilterContext *buffersrc_ctx;
static AVFilterGraph *filter_graph;

const char *filter_descr = "scale=78:24,transpose=cclock";
//const char *filter_descr = "drawtext=fontfile=arial.ttf:fontcolor=green:fontsize=30:text='FFMpeg Filter Demo'";

void Parse_Args(int argc, char **argv)
{
    pInput_File_Name  = argv[1];
    pOutput_File_Name = argv[2];

    width  = atoi(argv[3]);
    height = atoi(argv[4]);

    pfdInput = fopen(pInput_File_Name, "rb+");
    if(!pfdInput){
        fprintf(stderr, "open input file fail\n");
        exit(1);
    }
    pfdOutput = fopen(pOutput_File_Name, "wb+");
    if(!pfdOutput){
        fprintf(stderr, "open output file fail\n");
        exit(1);
    }
}

static int init_filters(const char *filter_descr)
{
    int ret;
    char args[512];
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();
    if(!outputs || !inputs || !filter_graph){
        ret = AVERROR(ENOMEM);
        goto end;
    }

    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts; 

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             width, height,AV_PIX_FMT_YUV420P,1,25,1,1);
    
    //buffer video source:the decoded frames from the decoder will be inserted here
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n"); 
        goto end;
    }

    //buffer video sink:to terminate the filter chain
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "in",
                                       NULL, NULL, filter_graph);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n"); 
        goto end;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name        = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;
    
    if((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                      &inputs, &outputs, NULL)) < 0) 
       goto end;
    if((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
       goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

void init_fram_in_out(AVFrame **framein, AVFrame **frameout,
                      unsigned char **frame_buffer_in, unsigned char **frame_buffer_out,
                      int framewidth, int frameheight)
{
    *framein = av_frame_alloc();
    *frame_buffer_in = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, framewidth, frameheight, 1));
    av_image_fill_arrays((*framein)->data, (*framein)->linesize, *frame_buffer_in, AV_PIX_FMT_YUV420P, framewidth, frameheight, 1);

    *frameout = av_frame_alloc();
    *frame_buffer_out = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, framewidth, frameheight, 1));
    av_image_fill_arrays((*frameout)->data, (*frameout)->linesize, *frame_buffer_out, AV_PIX_FMT_YUV420P, framewidth, frameheight, 1);

    (*framein)->width = framewidth;
    (*framein)->height = frameheight;
    (*framein)->format = AV_PIX_FMT_YUV420P;
}

int read_yuv_data_to_buf(unsigned char *frame_buffer_in, FILE *pfdInput, AVFrame **frameIn)
{
    AVFrame *pFrameIn = *frameIn;
    int framesize = width * height * 3 / 2;

    if(fread(frame_buffer_in, 1, framesize, pfdInput) != framesize)
    { return 0;}

    pFrameIn->data[0] = frame_buffer_in;
    pFrameIn->data[1] = pFrameIn->data[0] + width * height;
    pFrameIn->data[2] = pFrameIn->data[1] + width * height / 4;

    return 1;
}

int add_frame_to_filter(AVFrame *frameIn)
{
    if(av_buffersrc_add_frame(buffersrc_ctx, frameIn) < 0)
        return 0;
    return 1;
}

int get_frame_from_filter(AVFrame **frameout)
{
    if(av_buffersink_get_frame(buffersink_ctx, *frameout) < 0)
        return 0;
    return 1;
}

void write_yuv_to_outfile(const AVFrame *frame_out, FILE *pfdOutput)
{
    if(frame_out->format == AV_PIX_FMT_YUV420P)
    {
        for(int i=0;i<frame_out->height;i++) {  
            fwrite(frame_out->data[0]+frame_out->linesize[0]*i,1,frame_out->width,pfdOutput);  
        }  
        for(int i=0;i<frame_out->height/2;i++) {  
            fwrite(frame_out->data[1]+frame_out->linesize[1]*i,1,frame_out->width/2,pfdOutput);  
        }  
        for(int i=0;i<frame_out->height/2;i++) {  
            fwrite(frame_out->data[2]+frame_out->linesize[2]*i,1,frame_out->width/2,pfdOutput);  
        }  
    }
}

int main(int argc, char **argv)
{
    int ret; 
    AVFrame *frame_in  = NULL;
    AVFrame *frame_out = NULL;
    unsigned char *frame_buffer_in  = NULL;
    unsigned char *frame_buffer_out = NULL;

    //get the input arguments
    //(input and output file and width and height)
    Parse_Args(argc, argv);

    av_register_all();
    avfilter_register_all();

    if((ret = init_filters(filter_descr)) != 0){
        printf("init filters fail\n");   
        return ret;
    }

    init_fram_in_out(&frame_in, &frame_out, 
                     &frame_buffer_in, &frame_buffer_out,
                     width, height);

    while(read_yuv_data_to_buf(frame_buffer_in, pfdInput, &frame_in))
    {
        //put the frame to filter graph
        if(!add_frame_to_filter(frame_in))
        {
            printf("Error while adding frame\n");
            exit(1);
        }

        //get the frame from the filter graph
        if(!get_frame_from_filter(&frame_out))
        {
            printf("Error while getting frame\n");
            exit(1);
        }

        write_yuv_to_outfile(frame_out, pfdOutput);

        printf("Process 1 frame\n");
        av_frame_unref(frame_out);
    }

    return 0;
}
