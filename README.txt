ImageMagick Coder Kit

Use this kit to add support for additional image formats to ImageMagick.  We
include support for the MGK format which is simply the image width and height
followed by RGB pixels.  To build and install, make sure ImageMagick is
installed as a shared library with module support.  Next, unpack the kit into
the top-level ImageMagick source folder and type

  export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
  cd ImageMagick-7.?.?.?/MagickCoderKit-1.0.3
  ./configure
  make
  make install

to verify, type

  identify -list format

and look for the MGK tag.  Now verify the support works with

  convert logo: logo.mgk
  display logo.mgk

To support your own image format, follow these steps.  Assume your new
format is abbreviated SANS:

  1. Move the mgk.c module to sans.c.
  2. Globally replace 'MGK' with 'SANS' in sans.c.
  3. Globally replace 'mgk' with 'sans' in sans.c and Makefile.am.
  4. Edit sans.c and modify ReadSANSImage(), RegisterSANSImage(), and
     WriteSANSImage() to suit your image format.
  5. Rebuild the configuration files, type

       automake
       autoconf

  6. Build and install support for your image format:

       ./configure
       make
       make install
