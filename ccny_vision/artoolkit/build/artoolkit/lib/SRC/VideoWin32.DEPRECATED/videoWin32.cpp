/*******************************************************
 *
 * Author: Hirokazu Kato, Atsishi Nakazawa
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *         nakazawa@inolab.sys.es.osaka-u.ac.jp
 *
 * Revision: 4.1
 * Date: 99/07/16
 *
 * Adapted for Win32 by Robert Blanding, August 31, 1999
 *
*******************************************************/

// define FLIPPED if your video images need to be inverted
//#define FLIPPED

#include <string.h>
#include <AR/video.h>

#include <VisImSrc.h> // MS VisSDK


static int     open_flag = 0;
static void    error_exit(void);

// Vision SDC globals for the image sequence
CVisSequence<CVisRGBABytePixel>  sequence;
CVisRGBAByteImage                image;
#ifdef FLIPPED
CVisRGBAByteImage                flipped_image;
#endif



int  arVideoOpen(char *config)
{
    if( open_flag == 1 ) return(0);

    /* Connect to the VFW camera */
    VisAddProviderRegEntryForVFW();

    VisFindImageSource(sequence);

    if (!(sequence.HasImageSource()) || !(sequence.ImageSource().IsValid()))
        error_exit();

    // Don't use continuous grab since it chews up the processor like crazy
    sequence.ImageSource().SetUseContinuousGrab(false);

    // Grab an initial image
    if (!sequence.Pop(image, 40000))
    printf("Error in v_open: Couldn't get the image (init)\n");

    printf("Size = [%d, %d]\n", image.Width(), image.Height());


    open_flag = 1;
	
#ifdef FLIPPED
    // memory for a flipped copy of the image (for the USB camera)
    flipped_image.Allocate(image.Width(), image.Height());
#endif

    return(0);
}

int arVideoClose(void)
{
    if( open_flag == 0 ) return(-1);

    return(0);
}


int        arVideoDispOption( void )

{

	return 0;

}


int arVideoInqSize( int *x, int *y )
{
    if( open_flag == 0 ) return(-1);

    *x = image.Width();
    *y = image.Height();

    return(0);
}

unsigned char *arVideoGetImage( void )
{
#ifdef FLIPPED
    int             i, j;
#endif

    if( open_flag == 0 )              return(NULL);

    if (!sequence.Pop(image, 20000)){
        printf("Couldn't get the image\n");
        return NULL;
    }

#ifdef FLIPPED
    // make a flipped copy
    for (j = 0; j < image.Height(); j++){
        for (i = 0; i < image.Width(); i++){
            flipped_image.Pixel(i,(image.Height()-1-j)).SetR( image.Pixel(i, j).R() );
            flipped_image.Pixel(i,(image.Height()-1-j)).SetG( image.Pixel(i, j).G() );
            flipped_image.Pixel(i,(image.Height()-1-j)).SetB( image.Pixel(i, j).B() );
            flipped_image.Pixel(i,(image.Height()-1-j)).SetA( image.Pixel(i, j).A() );
        }
    }
    return (unsigned char*)flipped_image.PbPixel(flipped_image.StartPoint());
#endif

//  printf("Size = [%d, %d]\n", image.Width(), image.Height());
    return (unsigned char*)image.PbPixel(image.StartPoint());

    return(NULL);
}

int        arVideoCapStart( void )

{

	return 0;

}

int        arVideoCapStop( void )

{

	return 0;

}

int        arVideoCapNext( void )
{

	return 0;

}


static void error_exit(void)
{
    printf("Error_exit in Win32video_sub\n");
    exit(0);
}
