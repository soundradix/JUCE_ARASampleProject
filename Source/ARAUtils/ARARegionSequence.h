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

    AudioFormatReader* newReader();

    // These methods need to be called by the document controller in its corresponding methods:
    static void willUpdatePlaybackRegionProperties (
        ARA::PlugIn::PlaybackRegion*,
        ARA::PlugIn::PropertiesPtr<ARA::ARAPlaybackRegionProperties>);
    static void didUpdatePlaybackRegionProperties (ARA::PlugIn::PlaybackRegion*);

private:
    class Reader;
    typedef SafeRef<ARARegionSequence> Ref;

    Ref::Ptr ref_;
    std::vector<Reader*> readers_;
    std::map<ARA::PlugIn::AudioSource*, int> sourceRefCount_;

    // Used to unlock old sequence for region in `didUpdatePlaybackRegionProperties`.
    ARARegionSequence* prevSequenceForNewPlaybackRegion_;
};
