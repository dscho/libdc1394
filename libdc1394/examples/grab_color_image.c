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
**-------------------------------------------------------------------------
**
**  $Log$
**  Revision 1.5.2.1  2005/02/13 07:02:47  ddouxchamps
**  Creation of the Version_2_0 branch
**
**  Revision 1.5  2004/01/20 04:12:27  ddennedy
**  added dc1394_free_camera_nodes and applied to examples
**
**  Revision 1.4  2003/09/26 16:08:36  ddennedy
**  add libtoolize with no tests to autogen.sh, suppress examples/grab_color_image.c warning
**
**  Revision 1.3  2003/09/23 13:44:12  ddennedy
**  fix camera location by guid for all ports, add camera guid option to vloopback, add root detection and reset to vloopback
**
**  Revision 1.2  2003/09/15 17:21:28  ddennedy
**  add features to examples
**
**  Revision 1.1  2003/09/02 23:42:36  ddennedy
**  cleanup handle destroying in examples; fix dc1394_multiview to use handle per camera; new example
**
**
**************************************************************************/

#include <stdio.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>

#define MAX_PORTS 4
#define MAX_RESETS 10

char *g_filename = "image.ppm";
u_int64_t g_guid = 0;

static struct option long_options[]={
	{"guid",1,NULL,0},
	{"help",0,NULL,0},
	{NULL,0,0,0}
	};

void get_options(int argc,char *argv[])
{
	int option_index=0;
	
	while (getopt_long(argc,argv,"",long_options,&option_index)>=0)
	{
		switch(option_index){ 
			/* case values must match long_options */
			case 0:
				sscanf(optarg, "%llx", &g_guid);
				break;
			default:
				printf( "\n"
					"%s - grab a color image using format0, rgb mode\n\n"
					"Usage:\n"
					"    %s [--guid=/dev/video1394/x] [filename.ppm]\n\n"
					"    --guid    - specifies camera to use (optional)\n"
					"                default = first identified on buses\n"
					"    --help    - prints this message\n"
					"    filename is optional; the default is image.ppm\n\n"
					,argv[0],argv[0]);
				exit(0);
		}
	}
	if (optind < argc)
		g_filename = argv[optind];
}


int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394camera_t camera;
  dc1394capture_t capture;
  struct raw1394_portinfo ports[MAX_PORTS];
  int numPorts = 0;
  int numCameras = 0;
  int i, j;
  raw1394handle_t handle;
  nodeid_t * camera_nodes = NULL;
  int found = 0;

  get_options(argc, argv);
  
  /*-----------------------------------------------------------------------
   *  Open ohci and asign handle to it
   *-----------------------------------------------------------------------*/
  handle = raw1394_new_handle();
  if (handle==NULL)
  {
    fprintf( stderr, "Unable to aquire a raw1394 handle\n\n"
             "Please check \n"
	     "  - if the kernel modules `ieee1394',`raw1394' and `ohci1394' are loaded \n"
	     "  - if you have read/write access to /dev/raw1394\n\n");
    exit(1);
  }
	/* get the number of ports (cards) */
  numPorts = raw1394_get_port_info(handle, ports, numPorts);
  raw1394_destroy_handle(handle);
  handle = NULL;
  
  for (j = 0; j < MAX_RESETS && found == 0; j++)
  {
    /* look across all ports for cameras */
    for (i = 0; i < numPorts && found == 0; i++)
    {
      if (handle != NULL)
        dc1394_destroy_handle(handle);
      handle = dc1394_create_handle(i);
      if (handle == NULL)
      {
        fprintf( stderr, "Unable to aquire a raw1394 handle for port %i\n", i);
        exit(1);
      }
      numCameras = 0;
      camera_nodes = dc1394_get_camera_nodes(handle, &numCameras, 0);
      if (numCameras > 0)
      {
        if (g_guid == 0)
        {
          /* use the first camera found */
          camera.node = camera_nodes[0];
	  camera.handle=handle;
	  capture.node=camera_nodes[0];
          if (dc1394_get_camera_info(&camera) == DC1394_SUCCESS)
            dc1394_print_camera_info(&camera);
          found = 1;
        }
        else
        {
          /* attempt to locate camera by guid */
          int k;
	  camera.handle=handle;
          for (k = 0; k < numCameras && found == 0; k++)
          {
	    camera.node = camera_nodes[k];
            if (dc1394_get_camera_info(&camera) == DC1394_SUCCESS)
            {
              if (camera.euid_64 == g_guid)
              {
                dc1394_print_camera_info(&camera);
                capture.node = camera_nodes[k];
                found = 1;
              }
            }
          }
        }
        if (found == 1)
        {
          /* camera can not be root--highest order node */
          if (capture.node == raw1394_get_nodecount(handle)-1)
          {
            /* reset and retry if root */
            raw1394_reset_bus(handle);
            sleep(2);
            found = 0;
          }
        }
        dc1394_free_camera_nodes(camera_nodes);
      } /* cameras >0 */
    } /* next port */
  } /* next reset retry */
  
  if (found == 0 && g_guid != 0)
  {
    fprintf( stderr, "Unable to locate camera node by guid\n");
    exit(1);
  }
  else if (numCameras == 0)
  {
    fprintf( stderr, "no cameras found :(\n");
    dc1394_destroy_handle(handle);
    exit(1);
  }
  if (j == MAX_RESETS)
  {
    fprintf( stderr, "failed to not make camera root node :(\n");
    dc1394_destroy_handle(handle);
    exit(1);
  }
  
  /*-----------------------------------------------------------------------
   *  setup capture
   *-----------------------------------------------------------------------*/
  if (dc1394_setup_capture(&camera,
                           0, /* channel */ 
                           FORMAT_VGA_NONCOMPRESSED,
                           MODE_640x480_RGB,
                           SPEED_400,
                           FRAMERATE_7_5,
                           &capture)!=DC1394_SUCCESS) 
  {
    fprintf( stderr,"unable to setup camera-\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    dc1394_release_camera(&capture);
    dc1394_destroy_handle(handle);
    exit(1);
  }
  
  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_start_iso_transmission(&camera)
      !=DC1394_SUCCESS) 
  {
    fprintf( stderr, "unable to start camera iso transmission\n");
    dc1394_release_camera(&capture);
    dc1394_destroy_handle(handle);
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  capture one frame
   *-----------------------------------------------------------------------*/
  if (dc1394_single_capture(&capture)!=DC1394_SUCCESS) 
  {
    fprintf( stderr, "unable to capture a frame\n");
    dc1394_release_camera(&capture);
    dc1394_destroy_handle(handle);
    exit(1);
  }

 /*-----------------------------------------------------------------------
   *  save image as 'Image.pgm'
   *-----------------------------------------------------------------------*/
  imagefile=fopen(g_filename, "w");

  if( imagefile == NULL)
  {
    perror( "Can't create output file");
    dc1394_release_camera(&capture);
    dc1394_destroy_handle(handle);
    exit( 1);
  }
  
    
  fprintf(imagefile,"P6\n%u %u\n255\n", capture.frame_width,
          capture.frame_height );
  fwrite((const char *)capture.capture_buffer, 1,
         capture.frame_height*capture.frame_width*3, imagefile);
  fclose(imagefile);
  printf("wrote: %s\n", g_filename);

  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_release_camera(&capture);
  dc1394_destroy_handle(handle);
  return 0;
}
