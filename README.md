# libqct
- new fast image compression
- qct uses Delta & Run length coding.
- it only compresses yuv color model.
## what is the difference between jpeg and qct
- jpeg uses dct for more compression and scarifying quality.
- QCT uses the original YUV data to compress, resulting in a little more size.
## what is the difference between png and qct
- png is a lossless image file, commpressing RGBA values so it could be large in size.
- png take lot of time en/decoding.
## other features after beta of qct 
- storing Exif on the extended header.
- en/decrypting the image file.
