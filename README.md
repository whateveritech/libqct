# libqct
- new fast image compression
- QCT uses Delta & Run length coding.
- it only compresses yuv color model.
- quantization is also supported for more compression.
## What is the difference between jpeg and qct
- jpeg uses DCT(Discrete cosine transform) for more compression and scarifying quality.
- QCT uses the original YUV data to compress, resulting in a little more size.
## What is the difference between png and qct
- png is a lossless image file, commpressing RGBA values so it could be large in size.
- png take lot of time en/decoding.
## Other features after beta of qct 
- storing Exif on the extended header.
- en/decrypting the image file.
## Limitation
- bigger size compered LZ77 and Deflate compression(but slower), compress QCT file in zip or 7z file for more compression.
- not supported by any opreting system or hardware yet.
## Licence
QCT is a open format and libqct is GNU GPL licensed
## Why QCT
- If you want fast and better quality than jpeg and more compression than png.
