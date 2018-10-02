#pragma once

#include "ARA_Library/PlugIn/ARAPlug.h"
#include "JuceHeader.h"

#include <vector>

#include "SafeRef.h"

// An ARA `AudioSource` with a `Reader` class implementing `juce::AudioFormatReader`.
//
// The user may create as many `Reader`s as they need, which will fail on `readSamples`
// when samples access is disabled or after the audio source has been deleted.
//
// The ARA document controller should create these objects in `doCreateAudioSource`,
// as well as invoke additional calls as described in the declaration below.
class ARAAudioSource : public ARA::PlugIn::AudioSource
{
public:
    ARAAudioSource (ARA::PlugIn::Document*, ARA::ARAAudioSourceHostRef);
    virtual ~ARAAudioSource();

    AudioFormatReader* newReader();

    // Needs to be called in the document controller's `didUpdateAudioSourceProperties` method.
    void didUpdateProperties();

    // Needs to be called in the document controller's `willEnableAudioSourceSamplesAccess` method.
    void willEnableSamplesAccess (bool enable);

    // Needs to be called in the document controller's `didEnableAudioSourceSamplesAccess` method.
    void didEnableSamplesAccess (bool enable);

private:
    class Reader;
    typedef SafeRef<ARAAudioSource> Ref;

    Ref::Ptr ref_;
    std::vector<Reader*> readers_;
};
