# libqct Library for encoding/decoding QCT written for C/C++
- QCT is a simple image file, with fast compression.
- QCT uses Delta & Run length coding.
- it only compresses yuv color model.
## What is the difference between jpeg and qct
- jpeg uses DCT(Discrete cosine transform) for more compression while scarifying quality.
- QCT uses the original YUV data to compress, resulting in a little more size.
## What is the difference between png and qct
- png is a lossless image file, commpressing RGBA values so it could be large in size.
- png take lot of time en/decoding.
## features on version 1 of qct 
- Added YUV444 & YUV422.
- More compression.
## Limitation
- bigger size compered LZ77 and Deflate compression(but slower), compress QCT file in zip or 7z file for more compression.
- not supported by any opreting system or hardware yet.
## Licence
QCT is a open format and libqct is GNU GPL licensed
## Why QCT
- If you want fast and better quality than jpeg and more compression than png.
## Releases
- Beta version 0 : Feb 6 2024
- Stable version 1 : Apr 6 2024
