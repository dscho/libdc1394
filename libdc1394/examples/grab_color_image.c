/**************************************************************************
**       Title: grab one color image using libdc1394
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**    Get one  image using libdc1394 and store it as portable pix map
**    (ppm). Based on 'grab_gray_image' from Olaf Ronneberger
**
**************************************************************************/

#include <stdio.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>
#include <libdc1394/dc1394_utils.h>
#include <stdlib.h>

#define IMAGE_FILE_NAME "image.ppm"

/*-----------------------------------------------------------------------
 *  Releases the cameras and exits
 *-----------------------------------------------------------------------*/
void cleanup_and_exit(dc1394camera_t *camera)
{
  dc1394_capture_stop(camera);
  dc1394_video_set_transmission(camera, DC1394_OFF);
  dc1394_free_camera(camera);
  exit(1);
}

int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394camera_t *camera, **cameras=NULL;
  uint_t numCameras, i;
  unsigned int width, height;
  //dc1394featureset_t features;

  /* Find cameras */
  int err=dc1394_find_cameras(&cameras, &numCameras);

  if (err!=DC1394_SUCCESS) {
    fprintf( stderr, "Unable to look for an IIDC camera\n\n"
             "Please check that\n"
	     "  - the kernel modules `ieee1394',`raw1394' and `ohci1394' are loaded \n"
	     "  - you have read/write access to /dev/raw1394\n\n");
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  get the camera nodes and describe them as we find them
   *-----------------------------------------------------------------------*/
  if (numCameras<1) {
    fprintf(stderr, "no cameras found :(\n");
    exit(1);
  }
  camera=cameras[0];
  printf("working with the first camera on the bus\n");
  
  // free the other cameras
  for (i=1;i<numCameras;i++)
    dc1394_free_camera(cameras[i]);
  free(cameras);

  /*-----------------------------------------------------------------------
   *  setup capture
   *-----------------------------------------------------------------------*/

  err=dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
  DC1394_ERR_RTN(err,"Could not set ISO speed");
  dc1394_video_set_mode(camera,DC1394_VIDEO_MODE_640x480_RGB8);
  DC1394_ERR_RTN(err,"Could not set video mode 640x480xRGB8");
  dc1394_video_set_framerate(camera,DC1394_FRAMERATE_7_5);
  DC1394_ERR_RTN(err,"Could not set framerate to 7.5fps");
  if (dc1394_capture_setup(camera)!=DC1394_SUCCESS) {
    fprintf( stderr,"unable to setup camera-\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    cleanup_and_exit(camera);
  }
  
  /*-----------------------------------------------------------------------
   *  report camera's features
   *-----------------------------------------------------------------------*/
  /*
  if (dc1394_get_camera_feature_set(camera,&features) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to get feature set\n");
  }
  else {
    dc1394_print_feature_set(&features);
  }
  */
  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera, DC1394_ON) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to start camera iso transmission\n");
    cleanup_and_exit(camera);
  }

  /*-----------------------------------------------------------------------
   *  Sleep until the camera effectively started to transmit
   *-----------------------------------------------------------------------*/
  dc1394switch_t status = DC1394_OFF;

  i = 0;
  while( status == DC1394_OFF && i++ < 5 ) {
    usleep(50000);
    if (dc1394_video_get_transmission(camera, &status)!=DC1394_SUCCESS) {
      fprintf(stderr, "unable to get transmision status\n");
      cleanup_and_exit(camera);
    }
  }

  if( i == 5 ) {
    fprintf(stderr,"Camera doesn't seem to want to turn on!\n");
    cleanup_and_exit(camera);
  }

  /*-----------------------------------------------------------------------
   *  capture one frame
   *-----------------------------------------------------------------------*/
  if (dc1394_capture(&camera,1)!=DC1394_SUCCESS) {
    fprintf( stderr, "unable to capture a frame\n");
    cleanup_and_exit(camera);
  }

  /*-----------------------------------------------------------------------
   *  Stop data transmission
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera,DC1394_OFF)!=DC1394_SUCCESS) {
    printf("couldn't stop the camera?\n");
  }
  
 /*-----------------------------------------------------------------------
  *  save image as 'Image.pgm'
  *-----------------------------------------------------------------------*/
  imagefile=fopen(IMAGE_FILE_NAME, "w");

  if( imagefile == NULL) {
    perror( "Can't create output file");
    cleanup_and_exit(camera);
  }
  
  dc1394_get_image_size_from_video_mode(camera, DC1394_VIDEO_MODE_640x480_RGB8,
          &width, &height);
  fprintf(imagefile,"P6\n%u %u\n255\n", width, height);
  fwrite((const char *)dc1394_capture_get_dma_buffer (camera), 1,
         height*width*3, imagefile);
  fclose(imagefile);
  printf("wrote: " IMAGE_FILE_NAME " (%d image bytes)\n",height*width*3);

  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  cleanup_and_exit(camera);
  return 0;
}
