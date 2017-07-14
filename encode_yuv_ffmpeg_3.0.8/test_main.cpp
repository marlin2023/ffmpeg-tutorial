#include <stdio.h>
#include "EncodeYuv.h"

int main(int argc ,char *argv[]){

	// read yuv file
	FILE *yuvFile = fopen("akiyo_cif.yuv" ,"rb");
	
    int pict_width = 352;
    int pict_height = 288;
    int y_size =  pict_width * pict_height;
		
    VideoEncoder *videoEncoder = new VideoEncoder();
    
    // init ecodec context 
    videoEncoder->init("tmp.mp4");

	// encode work 
	int ii;
    for (ii = 0; ii < 200; ii ++){
  
       fread(videoEncoder->video_raw_data_buf, 1, y_size*3/2, yuvFile);
       videoEncoder->encode_video_frame(videoEncoder->encoded_yuv_pict ,NULL);
    }
   
    // uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    // fwrite(endcode, 1, sizeof(endcode), fp);
    // fclose(fp);
    // 1.3 flush end write file trailer.
    printf("=========>7\n");
    av_write_trailer(videoEncoder->ptr_format_ctx);

	return 0;
}