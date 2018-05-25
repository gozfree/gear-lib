##libmp4parser
This is a simple libmp4parser library.

The implement of mp4 parser comes from vlc-2.2.6 with stream patch.

check memory leak
valgrind --leak-check=full ./test_libmp4parser test.mp4


a typical mp4 parser result is:
vlc -vvv test.mp4
```
...
[00007fe5e8c01818] core demux debug: looking for demux module matching "mp4": 66 candidates
[00007fe5e8c01818] mp4 demux debug: added fragment moov
[00007fe5e8c01588] mp4 stream debug: dumping root Box "root"
[00007fe5e8c01588] mp4 stream debug: |   + ftyp size 32 offset 0
[00007fe5e8c01588] mp4 stream debug: |   + moov size 29184 offset 32
[00007fe5e8c01588] mp4 stream debug: |   |   + mvhd size 108 offset 40
[00007fe5e8c01588] mp4 stream debug: |   |   + trak size 11625 offset 148
[00007fe5e8c01588] mp4 stream debug: |   |   |   + tkhd size 92 offset 156
[00007fe5e8c01588] mp4 stream debug: |   |   |   + mdia size 11525 offset 248
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   + mdhd size 32 offset 256
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   + hdlr size 52 offset 288
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   + minf size 11433 offset 340
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   + vmhd size 20 offset 348
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   + dinf size 36 offset 368
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + dref size 28 offset 376
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   |   + url  size 12 offset 392
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   + stbl size 11369 offset 404
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stsd size 229 offset 412
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   |   + hvc1 size 213 offset 428
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   |   |   + hvcC size 127 offset 514
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stts size 5488 offset 641
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stsz size 2756 offset 6129
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stsc size 28 offset 8885
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stco size 2752 offset 8913
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stss size 108 offset 11665
[00007fe5e8c01588] mp4 stream debug: |   |   + trak size 17443 offset 11773
[00007fe5e8c01588] mp4 stream debug: |   |   |   + tkhd size 92 offset 11781
[00007fe5e8c01588] mp4 stream debug: |   |   |   + mdia size 17343 offset 11873
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   + mdhd size 32 offset 11881
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   + hdlr size 45 offset 11913
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   + minf size 17258 offset 11958
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   + smhd size 16 offset 11966
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   + dinf size 36 offset 11982
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + dref size 28 offset 11990
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   |   + url  size 12 offset 12006
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   + stbl size 17198 offset 12018
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stsd size 102 offset 12026
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   |   + mp4a size 86 offset 12042
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   |   |   + esds size 50 offset 12078
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stts size 8520 offset 12128
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stsz size 4272 offset 20648
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stsc size 28 offset 24920
[00007fe5e8c01588] mp4 stream debug: |   |   |   |   |   |   + stco size 4268 offset 24948
[00007fe5e8c01588] mp4 stream debug: |   + free size 311016 offset 29216
[00007fe5e8c01588] mp4 stream debug: |   + mdat size 1587142 offset 340232
[00007fe5e8c01818] mp4 demux debug: ISO Media file (isom) version 0.
[00007fe5e8c01818] mp4 demux debug: found 2 tracks
[00007fe5e8c01818] mp4 demux debug: track[Id 0x1] read 684 chunk
...
```


with libmp4parser result is:
```
debug: found Box: ftyp size 32 0
debug: found Box: moov size 29184 32
debug: found Box: mvhd size 108 40
debug: read box: "mvhd" creation 719140d-23h:45m:22s modification 719140d-23h:45m:22s time scale 90000 duration 0d-00h:00m:22s rate 1.000000 volume 1.000000 next track id 3
debug: found Box: trak size 11625 148
debug: found Box: tkhd size 92 156
debug: read box: "tkhd" creation 719140d-23h:45m:22s modification 719140d-23h:45m:22s duration 23d-30h:00m:00s track ID 1 layer 0 volume 0.000000 rotation 0.000000 scaleX 1.000000 scaleY 1.000000 translateX 0.000000 translateY 0.000000 width 1920.000000 height 1080.000000. Matrix: 65536 0 0 0 65536 0 0 0 1073741824
debug: found Box: mdia size 11525 248
debug: found Box: mdhd size 32 256
debug: read box: "mdhd" creation 719140d-23h:45m:22s modification 719140d-23h:45m:22s time scale 90000 duration 23d-30h:00m:00s language    
debug: found Box: hdlr size 52 288
debug: read box: "hdlr" handler type: "vide" name: "xxx Smart HEVC"
debug: found Box: minf size 11433 340
debug: found Box: vmhd size 20 348
debug: read box: "vmhd" graphics-mode 0 opcolor (0, 0, 0)
debug: found Box: dinf size 36 368
debug: found Box: dref size 28 376
debug: found Box: url  size 12 392
debug: read box: "url" url: (null)
debug: read box: "dref" entry-count 1
debug: found Box: stbl size 11369 404
debug: found Box: stsd size 229 412
debug: found Box: hvc1 size 213 428
debug: found Box: hvcC size 127 514
debug: read box: "vide" in stsd 1920x1080 depth 24
debug: read box: "stsd" entry-count 1
debug: found Box: stts size 5488 641
debug: read box: "stts" entry-count 684
debug: found Box: stsz size 2756 6129
debug: read box: "stsz" sample-size 0 sample-count 684
debug: found Box: stsc size 28 8885
debug: read box: "stsc" entry-count 1
debug: found Box: stco size 2752 8913
debug: read box: "co64" entry-count 684
debug: found Box: stss size 108 11665
debug: read box: "stss" entry-count 23
debug: found Box: trak size 17443 11773
debug: found Box: tkhd size 92 11781
debug: read box: "tkhd" creation 719140d-23h:45m:22s modification 719140d-23h:45m:22s duration 23d-27h:01m:33s track ID 2 layer 0 volume 1.000000 rotation 0.000000 scaleX 1.000000 scaleY 1.000000 translateX 0.000000 translateY 0.000000 width 0.000000 height 0.000000. Matrix: 65536 0 0 0 65536 0 0 0 1073741824
debug: found Box: mdia size 17343 11873
debug: found Box: mdhd size 32 11881
debug: read box: "mdhd" creation 719140d-23h:45m:22s modification 719140d-23h:45m:22s time scale 90000 duration 23d-27h:01m:33s language eng
debug: found Box: hdlr size 45 11913
debug: read box: "hdlr" handler type: "soun" name: "xxx AAC"
debug: found Box: minf size 17258 11958
debug: found Box: smhd size 16 11966
debug: read box: "smhd" balance 0.000000
debug: found Box: dinf size 36 11982
debug: found Box: dref size 28 11990
debug: found Box: url  size 12 12006
debug: read box: "url" url: (null)
debug: read box: "dref" entry-count 1
debug: found Box: stbl size 17198 12018
debug: found Box: stsd size 102 12026
debug: found Box: mp4a size 86 12042
debug: read box: "soun" mp4 or qt1/2 (rest=50)
debug: found Box: esds size 50 12078
debug: found esds MPEG4ESDescr (34Bytes)
debug: found esds MP4DecConfigDescr (22Bytes)
debug: found esds MP4DecSpecificDescr (5Bytes)
debug: read box: "soun" in stsd channel 1 sample size 16 sample rate 48000.000000
debug: read box: "stsd" entry-count 1
debug: found Box: stts size 8520 12128
debug: read box: "stts" entry-count 1063
debug: found Box: stsz size 4272 20648
debug: read box: "stsz" sample-size 0 sample-count 1063
debug: found Box: stsc size 28 24920
debug: read box: "stsc" entry-count 1
debug: found Box: stco size 4268 24948
debug: read box: "co64" entry-count 1063
debug: found Box: free size 311016 29216
debug: skip box: "free"
debug: found Box: mdat size 1587142 340232
debug: skip box: "mdat"
search result box is root
ftyp.major_brand is isom
warn: cannot free box hvc1, type unknown
```

