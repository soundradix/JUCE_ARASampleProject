Project archived, not maintained!

This has been integrated into mainline ARA development and later into mainline JUCE - [ARAPluginDemo](https://github.com/soundradix/JUCE/blob/main/examples/Plugins/ARAPluginDemo.h)

ARASampleProject
================

A simple JUCE project to be used with JUCE ARA2 support.

It basically expose the concept of `juce::AudioFormatReader`(s) as a way to access audio samples freely.
It uses `juce::AudioThumbnail` as they do concurrent reads to the audio files.

It's mostly a visual plug-in. It doesn't do any audio processing.
