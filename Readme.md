# Matching compressor 

Matching Compressor is the up-to-three knees compressor/expander that provides the possibility to match dynamic characteristics of one audio to another. Namely, if you have a reference audio with your "ideal" dynamic parameters and an audio you want to sound similar then this plugin can analyze both of them and determine the compressor/expander parameters that will be the best match. 

There is a number of plugins (from the old kind DirectX Steinberg FreeFilter to the modern Fab Filter Pro-Q, Voxengo CurveEQ, iZotope Ozone etc.) that allow to make spectral snapshots of two different audio fragments and determine the EQ parameters that match the first one to the second. The idea of MatchCompressor however is to make the similar thing with a dynamic range instead of spectrum.

First of all, you can use Matching Compressor as a simple compressor with standard Threshold, Ratio, Knee Width, Gain, Attack and Release parameters. Besides, you can choose its envelope filter type (peak or RMS) and the way the plugin processes stereo signal (it can process channels separately, choose maximum absolute value or calculate mean value). If Ratio < 1 then it works as an expander, except that expanding affects only signals above threshold and doesn't affect signals below it (many expanders work in an opposite way).

But the main feature of this plugin is the ability to determine the best compressing/expanding parameters that make dynamic characteristics of one audio to be closest to another. To do this, it analyzes theirs volume level statistics. This feature can be useful, for example, for choosing mastering compression parameters for your mix if you have a reference audio.

Sources of audios can be either sound files or input buses. If you want to use an input bus you can just play a fragment in your DAW to make the plugin learn it's audio characteristics: the main bus is used for the destination audio (i.e. audio that should be processed) and the side chain is used for the reference audio.

If a more accurate dynamic curve is needed to be determined then the two- or three-compressors/expanders mode can be used (i.e. the plugin can process a signal with one, two or three compressor/expander knees).

If you want to process your material with zero attack and release (in other words, you want the plugin to react immediately without introducing any delay in compression) then the process of determining optimal parameters from the statistics is rather obvious. However, setting attack and release times to zero is obviously not recommended for any compressor. So if you want to receive the closest matching taking into account envelope filtering before compressing then before starting the matching process you should specify the desired attack and release values, choose an envelope filter type and stereo signal processing type (for mono files, it will not be used). It should be noted that the plugin doesn't try to guess the best attack and release values, they should be specified manually. In general, compressor's longer attack increases the overall loudness and longer release decreases it.

Concerning choosing appropriate settings, accurate determining the best knee width requires higher region numbers and, therefore, takes more time. If you want to reduce the time, then you may consider choosing the "hard knee" option that will allow you to set up lower region numbers without significant precision loss. 

With its creepy design and unclear use cases, Matching Compressor is the flagman of The Useless Tools line that you would never want to have onboard!

Have fun!