// Written and contributed by Sound Radix Ltd.

#pragma once

#include "ARA_Library/PlugIn/ARAPlug.h"
#include "JuceHeader.h"

#include <map>

#include "SafeRef.h"

// An ARA `RegionSequence` with a `Reader` class implementing `juce::AudioFormatReader`.
//
// The ARA document controller should create these objects in `doCreateRegionSequence`,
// and invoke methods described in declaration appropriately.
class ARARegionSequence : public ARA::PlugIn::RegionSequence
{
public:
    ARARegionSequence (ARA::PlugIn::Document*, ARA::ARARegionSequenceHostRef);
    ~ARARegionSequence();

    // If not given a `sampleRate` will figure it out from the first playback region within.
    // Playback regions with differing sample rates will be ignored.
    // Future alternative could be to perform resampling.
    AudioFormatReader* newReader (double sampleRate = 0.0);

    // These methods need to be called by the document controller in its corresponding methods:
    static void willUpdatePlaybackRegionProperties (
        ARA::PlugIn::PlaybackRegion*,
        ARA::PlugIn::PropertiesPtr<ARA::ARAPlaybackRegionProperties>);
    static void didUpdatePlaybackRegionProperties (ARA::PlugIn::PlaybackRegion*);

    // Is sample access enabled in all audio sources in sequence?
    bool isSampleAccessEnabled() const;

private:
    class Reader;
    typedef SafeRef<ARARegionSequence> Ref;

    Ref::Ptr ref_;

    std::map<ARA::PlugIn::AudioSource*, int> sourceRefCount_;

    // Used to unlock old sequence for region in `didUpdatePlaybackRegionProperties`.
    ARARegionSequence* prevSequenceForNewPlaybackRegion_;

   #if JUCE_DEBUG
    static bool stateUpdatePlaybackRegionProperties;
   #endif
};
