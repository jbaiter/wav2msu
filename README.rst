wav2msu
=======
Utility to create MSU1-compatible PCM files from RIFF Wave files.

Usage:
------
wav2msu [-o outfile] [-l looppoint] [-i introfile] FILE.wav

Converts wave-files to a MSU1-compatible format.
Input is required to be a RIFF WAVE file in 16bit, 44.1kHz, 2ch PCM format.
Set filename to '-' to read from stdin.


Arguments:
  -i <file.wav>            Put <file.wav> before the main input file.
  -l <looppoint>           Set sample (relative to beginning of input file)
                           from which to loop, decimal or hexadecimal (0xabcd)
  -o <outfile.pcm>         Outputs to given filename, default is stdout
  -h                       Print help

Examples:
---------
* To convert a WAV file to MSU1-PCM
    $ wav2msu -o test.pcm test.wav
* To convert a FLAC file to MSU1-PCM
    $ flac -d -c file.flac | wav2msu -o test.pcm -
* To convert an OGG Vorbis file to MSU1-PCM
    $ oggdec -Q -o - test.ogg | wav2msu -o test.pcm -


Copyright
---------
Licensed under the terms of the MIT license (see source for details).
(c) 2011 Johannes Baiter <johannes.baiter@web.de>
Based on C# code by Kawa <http://helmet.kafuka.org/thepile/Wav2msu>
