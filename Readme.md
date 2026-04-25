# Matching compressor 

**Matching Compressor** a multi-knee compressor/expander (up to three knees) that allows you to match the dynamic characteristics of one audio signal to another.

If you have a reference audio signal with your desired dynamic profile and another signal you want to shape similarly, the plugin can analyze both and determine compressor/expander parameters that best match the reference.

## Key features:

- Standard set of parameters: Gain, Threshold, Ratio, Knee Width, Attack, Release, along with Envelope Type and Stereo Processing mode

- Up to three compressor/expander knees 

- Automatic matching of dynamic parameters 

- Support for both audio files and input buses (main and sidechain) as sources for reference and destination audio 

- Graphical representation of the processing curve 

There is a number of plugins (from the old kind DirectX **Steinberg FreeFilter** to the modern **Fab Filter Pro-Q**, **Voxengo CurveEQ**, **iZotope Ozone** etc.) that allow to make spectral snapshots of two different audio fragments and determine the EQ parameters that match the first one to the second. The idea of **Matching Compressor** however is to make the similar thing with a dynamic range instead of spectrum.

First of all, you can use **Matching Compressor** as a simple compressor with standard Threshold, Ratio, Knee Width, Gain, Attack and Release parameters. Besides, you can choose its envelope filter type (peak or RMS) and the way the plugin processes stereo signal (it can process channels separately, choose maximum absolute value or calculate mean value). If Ratio < 1 then it works as an expander, except that expanding affects only signals above threshold and doesn't affect signals below it (many expanders work in an opposite way).

But the main feature of this plugin is the ability to determine the best compressing/expanding parameters that make dynamic characteristics of one audio to be closest to another. To do this, it analyzes theirs volume level statistics. This feature can be useful, for example, for choosing mastering compression parameters for your mix if you have a reference audio.

Sources of audios can be either sound files or input buses. If you want to use an input bus you can just play a fragment in your DAW to make the plugin learn it's audio characteristics: the main bus is used for the destination audio (i.e. audio that should be processed) and the side chain is used for the reference audio.

If a more accurate dynamic curve is needed to be determined then the two- or three-compressors/expanders mode can be used (i.e. the plugin can process a signal with one, two or three compressor/expander knees).

The User Guide can be found at: <https://github.com/justonem0reuser/MatchingCompressor/blob/master/Docs/User%20Guide.pdf>

The Algorithm Description can be found at: <https://github.com/justonem0reuser/MatchingCompressor/blob/master/Docs/methods.pdf>

With its creepy design and unclear use cases, **Matching Compressor** is the flagman of The Useless Tools line that you would never want to have onboard.

Have fun!
