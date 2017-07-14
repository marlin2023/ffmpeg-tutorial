extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include "libavutil/timestamp.h"

#include <libavutil/imgutils.h>	

}

#include "EncodeYuv.h"
#include "tp_global_var.h"
#include "tp_error_code.h"


VideoEncoder::VideoEncoder(){
	
	/*	output media file information */
	ptr_format_ctx   			= NULL;			//	AVFormatContext
	output_format 				= NULL;

	/*	video information */
	video_stream 				= NULL;
	video_encoder_id 			= AV_CODEC_ID_NONE;

	encoded_yuv_pict			= NULL;
	video_raw_data_buf 			= NULL;

	frame_count					= 0;		// must do it in here
}

VideoEncoder::~VideoEncoder(){

	//
	if(this->video_raw_data_buf){
		av_free(this->video_raw_data_buf);
		this->video_raw_data_buf = NULL;
	}

	//close codecs
	avcodec_close(this->video_stream->codec);

	//free
	avformat_free_context(this->ptr_format_ctx);
}


int VideoEncoder::init(const char* output_file){

	// register all codec and formats
	av_register_all();
    avformat_network_init();

    // init work .
    printf("===> in init function ,output_file = %s \n" ,output_file);
    printf("                                                        \n");
    
	// 1.set AVOutputFormat
    avformat_alloc_output_context2(&this->ptr_format_ctx, NULL, NULL, output_file);
    if (this->ptr_format_ctx == NULL) {
        printf("Could not deduce[推断] output format from file extension: using mpeg.\n");
        avformat_alloc_output_context2(&this->ptr_format_ctx, NULL, "mpeg", output_file);
        if(this->ptr_format_ctx == NULL){
        	 printf("Could not find suitable output format\n");
        	 return NOT_GUESS_OUT_FORMAT;
        }
    }
    
    // 2.in here ,if I get AVOutputFormat succeed ,the filed audio_codec and video_codec will be set default.
    this->output_format = this->ptr_format_ctx->oformat;

    /* 3.add  video stream 	*/
    this->video_stream = NULL;
    this->video_encoder_id = AV_CODEC_ID_H264; //h264
	this->videoWidth = VIDEO_WIDTH;
	this->videoHeight = VIDEO_HEIGHT;

    if (this->output_format->video_codec != AV_CODEC_ID_NONE) {

    	this->video_stream = add_video_stream(this->ptr_format_ctx, this->video_encoder_id);
    	if(this->video_stream == NULL){
    		printf("in output ,add video stream failed \n");
    		return ADD_VIDEO_STREAM_FAIL;
    	}
    }
    printf("=======================>1\n");
    
    
    /* 4. avcodec_alloc_frame	*/
    this->encoded_yuv_pict = av_frame_alloc();     // AVFrame *frame = av_frame_alloc();
    if(this->encoded_yuv_pict == NULL){
		printf("yuv_frame allocate failed %s ,%d line\n" ,__FILE__ ,__LINE__);
		return MEMORY_MALLOC_FAIL;
	}

    int size = av_image_get_buffer_size(
                                  AV_PIX_FMT_YUV420P,
                                  this->video_stream->codec->width ,
                                  this->video_stream->codec->height ,1);
    
    this->video_raw_data_buf = (uint8_t *)av_malloc(size);
    if(this->video_raw_data_buf == NULL){
    	printf("pict allocate failed ...\n");
    	return MEMORY_MALLOC_FAIL;
    }
    printf("======>video_raw_data_buf size=%d .\n" ,size);
    
    av_image_fill_arrays(this->encoded_yuv_pict->data ,this->encoded_yuv_pict->linesize ,
                         this->video_raw_data_buf ,
                         AV_PIX_FMT_YUV420P ,
                         this->video_stream->codec->width ,
                         this->video_stream->codec->height ,
                         1);
    
    /*output the file information */
    av_dump_format(this->ptr_format_ctx, 0, output_file, 1);
    printf("========= after av_dump_format.....\n" );
    
    
	// 6 open encoder codec
	int ret1 = this-> open_stream_codec();
	if(ret1 != 0){
		printf("==>in CEffectCompound init function ,open stream codec failed");
		return OUTPUT_INIT_FAIL;
	}

	//---------------------start
	// 7.write file header
    if (!(this->output_format->flags & AVFMT_NOFILE)) {		//for mp4 or mpegts ,this must be performed
	    if (avio_open(&(this->ptr_format_ctx->pb), output_file, AVIO_FLAG_WRITE) < 0) {
	        printf("Could not open '%s'\n", output_file);
	        return OPEN_MUX_FILE_FAIL;
	    }
    }
    
    // write the stream header, if any
    ret1 = avformat_write_header(this->ptr_format_ctx ,NULL);
    printf("==>avformat_write_header return value = %d \n" ,ret1);
    printf("           \n");
	//---------------------end
	return 0;
}


int VideoEncoder::encode_video_frame( AVFrame *pict ,FILE *fp) {
    
	//set chris_count
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    
    //encode the image
    int video_encoded_out_size;
    
    pict->format = AV_PIX_FMT_YUV420P;
    pict->width = VIDEO_WIDTH;
    pict->height = VIDEO_HEIGHT;
    pict->pts = frame_count++;
    
    int got_packet_ptr  = 0;
    video_encoded_out_size = avcodec_encode_video2(
                this->video_stream->codec,
                &pkt ,pict ,&got_packet_ptr);

    if (video_encoded_out_size < 0) {
        printf("Error encoding video frame\n");
        return VIDEO_ENCODE_ERROR;
    }

    if(got_packet_ptr > 0){

        printf("======>Write frame %d (size = %d ) \n" ,frame_count ,pkt.size);        
        /* rescale output packet timestamp values from codec to stream timebase */
		pkt.pts = av_rescale_q_rnd(pkt.pts, this->video_stream->codec->time_base, this->video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, this->video_stream->codec->time_base, this->video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, this->video_stream->codec->time_base, this->video_stream->time_base);
		pkt.stream_index = this->video_stream->index;
        av_write_frame(this->ptr_format_ctx, &pkt);

    }
    
	av_free_packet(&pkt);
	return 0 ;
}


//===========================================================
int VideoEncoder::open_video_stream (AVStream * st){

	AVCodec *video_encoder;

	//find video encoder
	video_encoder = avcodec_find_encoder(st->codec->codec_id);
	if(video_encoder == NULL){
		printf("in output ,open_video ,can not find video encode.\n");
		return NO_FIND_VIDEO_ENCODE;
	}
    
	//open video encode
	if(avcodec_open2(st->codec ,video_encoder ,NULL) < 0){

		printf("in open_video function ,can not open video encode.\n");
		return OPEN_VIDEO_ENCODE_FAIL;
	}

    printf("       \n");
    printf("===============>> Success Open Video Codec .\n");
    printf("       \n");
	return 0;

}


int VideoEncoder::open_stream_codec(){
    
    int ret ;
    ret = this->open_video_stream (this->video_stream);
    if(ret != 0){
        printf("====>open_video_stream failed ...\n");
        return OPEN_VIDEO_STREAM_FAIL;
    }
    return 0;
}


AVStream * VideoEncoder::add_video_stream (AVFormatContext *fmt_ctx ,enum AVCodecID codec_id){
    
    AVStream *st;
    AVCodecContext *avctx;
    // add video stream
    st = avformat_new_stream(fmt_ctx ,NULL);
    if(st == NULL){
        printf("in out file ,new video stream fimplicit dailed ..\n");
        return NULL;	//NEW_VIDEO_STREAM_FAIL;
    }
    
    //set the index of the stream
    st->id = ID_VIDEO_STREAM;

    //set AVCodecContext of the stream
    avctx = st->codec;
    
    avctx->codec_id = codec_id;
    avctx->codec_type = AVMEDIA_TYPE_VIDEO;
    
    avctx->bit_rate = VIDEO_BIT_RATE;
    avctx->width = VIDEO_WIDTH;
    avctx->height = VIDEO_HEIGHT;
    avctx->pix_fmt = AV_PIX_FMT_YUV420P;
    avctx->time_base.den = VIDEO_FRAME_RATE;
    avctx->time_base.num = 1;

	st->time_base.den = VIDEO_FRAME_RATE;
    st->time_base.num = 1;

    
    avctx->me_range = 16;
    avctx->qcompress = 0.6;
    avctx->qmin = 10;
    avctx->qmax = 26;
    avctx->max_qdiff = 4;
    avctx->thread_count = 6;
    
    //key frame
    avctx->keyint_min = 25;
    avctx->gop_size = VIDEO_FRAME_RATE;
    
    st->codec->profile = FF_PROFILE_H264_BASELINE;//FF_PROFILE_H264_MAIN;
    // some formats want stream headers to be separate(for example ,asfenc.c ,but not mpegts)
    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    
    return st;
}
