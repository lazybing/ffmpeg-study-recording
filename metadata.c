/*
 * @file
 * Show how the metadata API can be used in application programs
 * @example metadata.c
 */

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

int main(int argc, char **argv)
{
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag   = NULL;
    int ret;

    if(argc != 2){
        printf("usage:%s <input_file>\n"
               "example program to demostrate the use of the libavformat metadata API.\n", argv[0]); 
        return 1;
    }

    av_register_all();
    if((ret = avformat_open_input(&fmt_ctx, argv[1], NULL, NULL)))
        return ret;

    printf("AVDictionary count %d\n", 
           av_dict_count(fmt_ctx->metadata));

    while((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        printf("%s=%s\n", tag->key, tag->value);

    avformat_close_input(&fmt_ctx);

    return 0;
}

