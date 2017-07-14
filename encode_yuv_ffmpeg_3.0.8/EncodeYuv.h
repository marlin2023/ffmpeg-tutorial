#ifndef __ENCODE_YUV_H__
#define __ENCODE_YUV_H__

extern "C" {

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/error.h>
#include "libavutil/fifo.h"

}


class VideoEncoder {
public:
	VideoEncoder();
	~VideoEncoder();

	/*
	 * function  init ,this function will add video stream and audio stream for output .
	 * @param:	output_file			the output file name
	 *
	 * */
	int init(const char* output_file);

	/*
	 * function : open video stream codec .
	 * @param:	ptr_output_ctx 	 	a structure contain the output file information
	 *
	 * */
	int open_stream_codec();

	/*
	 * function : encode one video frame .
	 *
	 * */	
	int encode_video_frame( AVFrame *pict ,FILE *fp);
    

private:

	/* *
	 * function add_video_stream , add_video_stream failed return NULL.
	 * @param[in]:	fmt_ctx 	 		a handle of AVFormatContent for OutPutFile 
	 * @param[in]:	codec_id			the encoder id for current video stream
	 *
	 * */
	AVStream * add_video_stream (AVFormatContext *fmt_ctx ,enum AVCodecID codec_id);

	/* *
	 * function open_video_stream.
	 * @param[in]			st 	 		a handle of video stream for output file. 
	 * return value ,if success return 0.
	 * */
	int open_video_stream (AVStream * st);


public:
	//output file
	AVFormatContext *ptr_format_ctx;
	AVOutputFormat *output_format;

	/*=======	video information =========*/
	AVStream *video_stream;
	enum AVCodecID video_encoder_id;

	// data pass to video encoder
	AVFrame *encoded_yuv_pict;
	uint8_t * video_raw_data_buf;

	//the input stream
	double sync_ipts;
	double base_ipts;

	// use to count for video frame
	int frame_count ;

	int videoWidth;
	int videoHeight;

};


#endif

