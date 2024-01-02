#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/colormap-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/static.h"
#include "MagickCore/string_.h"
#include "MagickCore/module.h"
#include "MagickCore/transform.h"
#include "MagickCore/attribute.h"
#include <zlib.h>

typedef struct _ASEHeader
{
  uint32_t filesize;
  uint16_t magic;
  uint16_t frames;
  uint16_t width;
  uint16_t height;
  uint16_t bitsperpixel;
  uint32_t flags;
  uint16_t speed;
  uint32_t zero1;
  uint32_t zero2;
  uint8_t transparentIndex;
  uint8_t ignore1, ignore2, ignore3;
  uint16_t numcolours;
  uint8_t pixelwidth;
  uint8_t pixelheight;
  int16_t xposgrid;
  int16_t yposgrid;
  uint16_t gridwidth;
  uint16_t gridheight;
} ASEHeader;

typedef struct _ASEPaletteColour
{
  uint16_t flags;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;
  const char* pName;
} ASEPaletteColour;

static const char* ReadString( Image* _image )
{
  uint16_t length;
  char* pName;
  int n;

  length = ReadBlobLSBShort( _image );
  pName = (char*)AcquireMagickMemory( length+1 );
  for( n=0; n<length; ++n) {
    pName[n] = ReadBlobByte(_image);
  } // end for
  pName[n] = '\0';

  return (const char*)pName;
} // end ReadString

static void PrintMemory( void* _pMemory, int _size, int _width)
{
  int n, m;
  for( n=0; n<_size; n+=_width) {
    printf( "%08x :: ", n);
    for( m=0; m<_width && (m+n)<_size ; ++m) {
      printf( "%02x ", ((uint8_t*)_pMemory)[m+n] );
    } // end for
    printf( "\n");
  } // end for
} // end PrintMemory

// 0x2007
static void ChunkColorProfile( Image* _image, ASEHeader* _header, uint32_t _sizeChunk, ExceptionInfo *_exception)
{
  uint16_t typeColor;
  uint16_t flags;
  int32_t gamma;

  typeColor = ReadBlobLSBShort( _image );
  flags = ReadBlobLSBShort(_image);
  gamma = ReadBlobLSBSignedLong(_image);


  if (flags & 0x1) {
    _image->gamma = (double)gamma/65536.0;
    SetImageColorspace(_image, sRGBColorspace, _exception);
  } // end if
  else {
    _image->gamma = 1.0;
    SetImageColorspace(_image, RGBColorspace, _exception);
  } // end if

  printf( "ChunkColorProfile :: type=%04x, flags=%04x, gamma=%08x\n", typeColor, flags, gamma);
} // end ChunkColorProfile

// 0x2019 
static void ChunkPalette( Image* _image, ASEHeader* _header, uint32_t _sizeChunk, ExceptionInfo *_exception)
{
  uint32_t numEntries;
  uint32_t firstIndex;
  uint32_t lastIndex;
  uint32_t  zeros[2];
  ASEPaletteColour col;
  int n;

  numEntries = ReadBlobLSBLong( _image );
  firstIndex = ReadBlobLSBLong( _image );
  lastIndex = ReadBlobLSBLong( _image );
  zeros[0] = ReadBlobLSBLong( _image );
  zeros[1] = ReadBlobLSBLong( _image );
  printf( "ChunkPalette :: numEntries=%d, firstIndex=%d, lastIndex=%d\n", numEntries, firstIndex, lastIndex );
  for( n=0; n<lastIndex -firstIndex+1; ++n) {
    int index = firstIndex + n;
    col.flags = ReadBlobLSBShort( _image );
    col.red = ReadBlobByte( _image );
    col.green = ReadBlobByte( _image );
    col.blue = ReadBlobByte( _image );
    col.alpha = ReadBlobByte( _image );
    if ((col.flags & 0x1) != 0) {
      col.pName = ReadString( _image );
    } // end if
    else {
      col.pName = NULL;
    } // end else
    printf( "%d :: flags=%d, red=%d, green=%d, blue=%d, alpha=%d, name=%s\n", n, col.flags, col.red, col.green, col.blue, col.alpha, col.pName);

    _image->colormap[ index ].red = (MagickRealType)ScaleCharToQuantum( col.red );
    _image->colormap[ index ].green = (MagickRealType)ScaleCharToQuantum( col.green );
    _image->colormap[ index ].blue = (MagickRealType)ScaleCharToQuantum( col.blue );
    _image->colormap[ index ].alpha = (MagickRealType)ScaleCharToQuantum( col.alpha );

  } // end for
  (void)zeros[0];
  (void)zeros[1];
} // end ChunkPalette

// 0x0004
static void ChunkOldPalette( Image* _image, ASEHeader* header, uint32_t _sizeChunk, ExceptionInfo *_exception)
{
  uint16_t numPackets;
  uint8_t numSkip;
  uint8_t numColours;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  int n, m;
  numPackets = ReadBlobLSBShort( _image );
  printf( "ChunkOldPalette :: numPackets=%d\n", numPackets);
  for( n=0; n<numPackets; ++n) {
    numSkip = ReadBlobByte( _image );
    numColours = ReadBlobByte( _image );
    printf( "#%d :: numSkip=%d, numColours=%d\n", n, numSkip, numColours );
    for( m=0; m<numColours; ++m) {
      red = ReadBlobByte( _image );
      green = ReadBlobByte( _image );
      blue = ReadBlobByte( _image );
      printf( "%d :: red=%d, green=%d, blue=%d\n", m, red, green, blue );
    } // end for
  } // end for
} // end ChunkOldPalette

static void ChunkLayer( Image* _image, ASEHeader* header, uint32_t _sizeChunk, ExceptionInfo *_exception)
{
  uint16_t flags;
  uint16_t type;
  uint16_t childLevel;
  uint16_t width;
  uint16_t height;
  uint16_t blendMode;
  uint8_t  opacity;
  uint8_t  zeros[3];
  const char* pName;
  uint32_t tilesetIndex;

  flags = ReadBlobLSBShort( _image );
  type = ReadBlobLSBShort( _image );
  childLevel = ReadBlobLSBShort( _image );
  width = ReadBlobLSBShort( _image );
  height = ReadBlobLSBShort( _image );
  blendMode = ReadBlobLSBShort( _image );
  opacity = ReadBlobByte( _image );
  zeros[0] = ReadBlobByte( _image );
  zeros[1] = ReadBlobByte( _image );
  zeros[2] = ReadBlobByte( _image );
  pName = ReadString(_image);
  tilesetIndex = 0xffffffff;
  if (type == 2) {
    tilesetIndex = ReadBlobLSBLong(_image);
  } // end if
  printf( "ChunkLayer :: flags=%04x(%d), type=%d, childLevel=%d, width=%d, height=%d, blendMode=%d, opacity=%d, name=%s, tilesetIndex=%d\n", 
            flags, flags, type, childLevel, width, height, blendMode, opacity, pName, tilesetIndex );
  (void)zeros[0];
  (void)zeros[1];
  (void)zeros[2];
} // end ChunkLayer

static void SetASERect( Image* _image, ASEHeader* _header, uint8_t* _pPixels, int _xoffs, int _yoffs, int _width, int _height, ExceptionInfo* _exception)
{
  int xx, yy;
  uint8_t* pP, *p;
  Quantum* q;
  int strideS, pixS, pixD;


  pixS = _header->bitsperpixel/8;
  //printf( "pixS=%d\n", pixS);
  strideS = _width * pixS;
  pP = _pPixels; //+ (_xoffs * pixS) + (_yoffs * strideS);

  for( yy=0; yy<_height; ++yy, pP += strideS) {
    p = pP;
    q = GetAuthenticPixels(_image, _xoffs, _yoffs+yy, _width, 1, _exception);
    pixD = GetPixelChannels(_image);
    printf( "%d :: ", yy);

    if (q != (Quantum*)NULL) {
      for( xx=0; xx<_width; ++xx, p += pixS, q += pixD) {

        //switch( _header->bitsperpixel ) {
        //case 8:
          {
            uint8_t b = *p;
            //printf( "%02x ", b);
            //Quantum pix = ScaleCharToQuantum( b );
            ssize_t index = ConstrainColormapIndex( _image, b, _exception );
            SetPixelViaPixelInfo( _image, &_image->colormap[index], q );
          } // end block 
          //break;
        //} // end switch

      } // end for
    } // end if

    SyncAuthenticPixels(_image, _exception );
    //printf( "\n");
  } // end for

} // end SetASERect

static void ChunkCel( Image* _image, ASEHeader* _header, uint32_t _sizeChunk, ExceptionInfo *_exception)
{
  uint16_t layerIndex;
  int16_t  xpos;
  int16_t  ypos;
  uint8_t  opacity;
  uint16_t type;
  int16_t  zIndex;
  uint8_t  zeros[5];
  MagickOffsetType offsStart;

  offsStart = TellBlob(_image);
  layerIndex = ReadBlobLSBShort( _image );
  xpos = ReadBlobLSBSignedShort( _image );
  ypos = ReadBlobLSBSignedShort( _image );
  opacity = ReadBlobByte( _image );
  type = ReadBlobLSBShort( _image );
  zIndex = ReadBlobLSBSignedShort( _image );
  zeros[0] = ReadBlobByte( _image );
  zeros[1] = ReadBlobByte( _image );
  zeros[2] = ReadBlobByte( _image );
  zeros[3] = ReadBlobByte( _image );
  zeros[4] = ReadBlobByte( _image );

  printf( "ChunkCel :: layerIndex=%d, xpos=%d, ypos=%d, opacity=%d, type=%d, zIndex=%d\n", layerIndex, xpos, ypos, opacity, type, zIndex );
  switch( type )
  {
  case 0: // raw image data (unused, compressed image is preferred)
  case 1: // linked cel
    break;
  case 2: // compressed image
    {
      uint16_t width = ReadBlobLSBShort(_image);
      uint16_t height = ReadBlobLSBShort( _image );
      // read in the rest of the chunk into memory so we can uncompress
      int numBytes = _sizeChunk - (TellBlob(_image) - offsStart);
      uint8_t* pCBuffer = (uint8_t*)AcquireMagickMemory( numBytes );
      unsigned long uncompressedSize = width*height*_header->bitsperpixel/8;
      uint8_t* pPixels = (uint8_t*)AcquireMagickMemory( uncompressedSize );
      unsigned long uncompressReturn;
      ReadBlob( _image, numBytes, pCBuffer );
      uncompressReturn = uncompress( pPixels, &uncompressedSize, pCBuffer, numBytes);

      PrintMemory( pCBuffer, numBytes, 16);
      printf( "width=%d, height=%d, numBytes=%d, uncompressReturn=%ld\n", width, height, numBytes, uncompressReturn);
      PrintMemory( pPixels, uncompressedSize, width*_header->bitsperpixel/8 );

      // if we have uncompressed OK then lets start setting the bitmap up
      if (uncompressReturn == Z_OK) {

        // set the pixels in the ImageMagick image
        SetASERect( _image, _header, pPixels, xpos, ypos, width, height, _exception);

      } // end if


      // clean up this bit
      RelinquishMagickMemory(pCBuffer);
      RelinquishMagickMemory(pPixels);
    } // end block
    break;
  case 3: // compressed tilemap
    break;
  }
  (void)zeros[0];
  (void)zeros[1];
  (void)zeros[2];
  (void)zeros[3];
  (void)zeros[4];
} // end ChunkCel

static Image *ReadASEImage(const ImageInfo *image_info, ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    status;

  ASEHeader header;
  uint32_t currsize;
  uint32_t framesize;
  uint16_t framemagic;
  uint16_t numberOfChunks;
  uint16_t frameDuration;
  uint16_t zero1;
  uint32_t newNumberOfChunks;
  uint32_t chunkSize;
  uint16_t chunkType;  
  int n;
  MagickOffsetType curroffset;
  MagickOffsetType currChunkOffset;

  printf( "ReadASE :: image_info = %p, Exception = %p\n", image_info, exception );
  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImage(image_info,exception);
  image->columns=0;
  image->rows=0;
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }

  header.filesize = ReadBlobLSBLong(image);
  header.magic = ReadBlobLSBShort(image);
  header.frames = ReadBlobLSBShort(image);
  header.width = ReadBlobLSBShort(image);
  header.height = ReadBlobLSBShort(image);
  header.bitsperpixel = ReadBlobLSBShort(image);
  header.flags = ReadBlobLSBLong(image);
  header.speed = ReadBlobLSBShort(image);
  header.zero1 = ReadBlobLSBLong(image);
  header.zero2 = ReadBlobLSBLong(image);
  header.transparentIndex = ReadBlobByte(image);
  header.ignore1 = ReadBlobByte(image);
  header.ignore2 = ReadBlobByte(image);
  header.ignore3 = ReadBlobByte(image);
  header.numcolours = ReadBlobLSBShort(image);
  header.pixelwidth = ReadBlobByte(image);
  header.pixelheight = ReadBlobByte(image);
  header.xposgrid = ReadBlobLSBShort(image);
  header.yposgrid = ReadBlobLSBShort(image);
  header.gridwidth = ReadBlobLSBShort(image);
  header.gridheight = ReadBlobLSBShort(image);

  printf( "filesize = %d\n", header.filesize);
  printf( "magic = %04x\n", header.magic);
  printf( "frames = %d\n", header.frames);
  printf( "width = %d\n", header.width);
  printf( "height = %d\n", header.height);
  printf( "bitsperpixel = %d\n", header.bitsperpixel);
  printf( "flags = %08x\n", header.flags);
  printf( "speed = %d\n", header.speed);
  printf( "transparentindex = %d\n", header.transparentIndex);
  printf( "numcolours = %d\n", header.numcolours);
  printf( "pixelwidth = %d\n", header.pixelwidth);
  printf( "pixelheight = %d\n", header.pixelheight);
  printf( "xposgrid = %d\n", header.xposgrid);
  printf( "yposgrid = %d\n", header.yposgrid);
  printf( "gridwidth = %d\n", header.gridwidth);
  printf( "gridheight = %d\n", header.gridheight);

  SeekBlob(image, 128, SEEK_SET);


  image->rows = header.height;
  image->columns = header.width;
  image->alpha_trait = BlendPixelTrait;
  image->rendering_intent = PerceptualIntent;
  image->depth = header.bitsperpixel;
  image->depth = GetImageQuantumDepth(image, MagickFalse );
  image->interlace = NoInterlace;
  image->compression = NoCompression;

  if (AcquireImageColormap( image, header.numcolours, exception ) == MagickFalse) {
    ThrowReaderException( ResourceLimitError, "MemoryAllocationFailed" );
  } // end if


  currsize = header.filesize - 128;
  while( currsize > 0) {

    curroffset = TellBlob( image );

    // get all the frames
    framesize = ReadBlobLSBLong(image);
    framemagic = ReadBlobLSBShort(image);
    printf( "------------------------------------------------\n" );
    printf( "framesize = %d\n", framesize);
    printf( "framemagic = %04x\n", framemagic);


    numberOfChunks = ReadBlobLSBShort(image);
    frameDuration = ReadBlobLSBShort(image);
    zero1 = ReadBlobLSBShort(image);
    newNumberOfChunks = ReadBlobLSBLong(image);


    printf( "numberOfChunks =%d, newNumberOfChunks=%08x(%d)\n", numberOfChunks, newNumberOfChunks, newNumberOfChunks);
    for( n=0; n<newNumberOfChunks; ++n) {
      currChunkOffset = TellBlob(image);
      chunkSize = ReadBlobLSBLong(image);
      chunkType = ReadBlobLSBShort(image);
      printf( "chunk Type=%04x(%d), size=%08x(%d)\n", chunkType, chunkType, chunkSize, chunkSize );
      switch( chunkType ) {
      case 0x2007: // Color Profile Chunk
        ChunkColorProfile( image, &header, chunkSize, exception);
        break;
      case 0x2019: // Palette Chunk
        ChunkPalette( image, &header, chunkSize, exception);
        break;
      case 0x0004: // Old Palette Chunk
        ChunkOldPalette( image, &header, chunkSize, exception);
        break;
      case 0x2004: // Layer Chunk
        ChunkLayer( image, &header, chunkSize, exception );
        break;
      case 0x2005: // Cel Chunk
        ChunkCel( image, &header, chunkSize, exception );
        break;
      } // end switch
      SeekBlob( image, currChunkOffset + chunkSize, SEEK_SET );
    } // end for

    // next frame
    SeekBlob(image, curroffset+framesize, SEEK_SET);
    currsize -= framesize;
  } // end while

  (void)zero1;
  (void)frameDuration;
  return image;
} // end ReadASEImage


static MagickBooleanType WriteASEImage(const ImageInfo *image_info,Image *image, ExceptionInfo *exception)
{
  printf( "WriteASE :: image_info = %p, image = %p, Exception = %p\n", image_info, image, exception );
  return MagickTrue;
} // end WriteASEImage


static MagickBooleanType IsASE(const unsigned char *magick,const size_t length)
{
  printf( "IsASE :: magick = %p, len = %ld\n", magick, length );
  return(MagickFalse);
} // end IsASE


ModuleExport size_t RegisterASEImage(void)
{
  MagickInfo
    *entry;

  entry=AcquireMagickInfo("ASE","ASE","Animated Sprite Editor file format");
  entry->decoder=(DecodeImageHandler *) ReadASEImage;
  entry->encoder=(EncodeImageHandler *) WriteASEImage;
  entry->magick=(IsImageFormatHandler *) IsASE;
  entry->flags^=CoderAdjoinFlag;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->mime_type=ConstantString("application/octet-stream");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

ModuleExport void UnregisterASEImage(void)
{
  (void) UnregisterMagickInfo("ASE");
}
