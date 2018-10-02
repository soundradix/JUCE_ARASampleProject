#include "ARARegionSequence.h"

#include "ARAAudioSource.h"

class ARARegionSequence::Reader : public AudioFormatReader
{
    friend class ARARegionSequence;

public:
    Reader (ARARegionSequence*);
    virtual ~Reader();

    bool readSamples (
        int** destSamples,
        int numDestChannels,
        int startOffsetInDestBuffer,
        int64 startSampleInFile,
        int numSamples) override;

    // Update channelCount, sampleRate and length according to region/source info.
    void updateProperties (const ARA::PlugIn::PlaybackRegion&, const ARA::PlugIn::AudioSource&);

private:
    Ref::Ptr ref_;
    std::map<ARA::PlugIn::AudioSource*, AudioFormatReader*> sourceReaders_;
    AudioSampleBuffer buf_;
};

ARARegionSequence::ARARegionSequence (ARA::PlugIn::Document* document, ARA::ARARegionSequenceHostRef hostRef)
    : ARA::PlugIn::RegionSequence (document, hostRef)
{
    ref_ = new Ref (this);
    prevSequenceForNewPlaybackRegion_ = nullptr;
}

ARARegionSequence::~ARARegionSequence()
{
    ref_->reset();
}

AudioFormatReader* ARARegionSequence::newReader()
{
    return new Reader (this);
}

void ARARegionSequence::willUpdatePlaybackRegionProperties (
    ARA::PlugIn::PlaybackRegion* region,
    ARA::PlugIn::PropertiesPtr<ARA::ARAPlaybackRegionProperties> properties)
{
    ARARegionSequence* oldSequence = dynamic_cast<ARARegionSequence*> (region->getRegionSequence());
    ARARegionSequence* newSequence = dynamic_cast<ARARegionSequence*> (ARA::PlugIn::fromRef (properties->regionSequenceRef));
    jassert (newSequence != nullptr);
    jassert (newSequence->prevSequenceForNewPlaybackRegion_ == nullptr);
    newSequence->ref_->lock.enterWrite();
    newSequence->prevSequenceForNewPlaybackRegion_ = oldSequence;
    if (oldSequence == newSequence)
    {
        // Sequence did not change.
        return;
    }
    if (oldSequence != nullptr)
    {
        oldSequence->ref_->lock.enterWrite();
        ARA::PlugIn::AudioSource* source = region->getAudioModification()->getAudioSource();
        if (0 == --oldSequence->sourceRefCount_[source])
            for (auto& reader : oldSequence->readers_)
            {
                auto iter = reader->sourceReaders_.find (source);
                delete iter->second;
                reader->sourceReaders_.erase (iter);
            }
    }
}

void ARARegionSequence::didUpdatePlaybackRegionProperties (ARA::PlugIn::PlaybackRegion* region)
{
    ARARegionSequence* newSequence = dynamic_cast<ARARegionSequence*> (region->getRegionSequence());
    jassert (newSequence != nullptr);

    ARARegionSequence* oldSequence = newSequence->prevSequenceForNewPlaybackRegion_;
    newSequence->prevSequenceForNewPlaybackRegion_ = nullptr;

    ARAAudioSource* source = dynamic_cast<ARAAudioSource*> (region->getAudioModification()->getAudioSource());
    jassert (source != nullptr);

    if (newSequence != oldSequence && 0 == newSequence->sourceRefCount_[source]++)
        for (auto& reader : newSequence->readers_)
        {
            reader->updateProperties (*region, *source);
            jassert (reader->sourceReaders_.find (source) == reader->sourceReaders_.end());
            reader->sourceReaders_[source] = source->newReader();
        }
    else
        for (auto& reader : newSequence->readers_)
            reader->updateProperties (*region, *source);

    if (oldSequence != newSequence && oldSequence != nullptr)
        oldSequence->ref_->lock.exitWrite();
    newSequence->ref_->lock.exitWrite();
}

ARARegionSequence::Reader::Reader (ARARegionSequence* sequence)
    : AudioFormatReader (nullptr, "ARARegionSequenceReader")
    , ref_ (sequence->ref_)
{
    bitsPerSample = 32;
    usesFloatingPointData = true;
    numChannels = 0;
    lengthInSamples = 0;
    sampleRate = 0.0;

    Ref::ScopedAccess access (ref_);
    jassert (access);
    {
        ScopedWriteLock write (ref_->lock);
        sequence->readers_.push_back (this);
    }
    for (ARA::PlugIn::PlaybackRegion* region : sequence->getPlaybackRegions())
    {
        ARA::PlugIn::AudioModification* modification = region->getAudioModification();
        jassert (modification != nullptr);
        ARAAudioSource* source = dynamic_cast<ARAAudioSource*> (modification->getAudioSource());
        jassert (source != nullptr);

        updateProperties (*region, *source);

        if (sourceReaders_.find (source) == sourceReaders_.end())
            sourceReaders_[source] = source->newReader();
    }
}

ARARegionSequence::Reader::~Reader()
{
    if (Ref::ScopedAccess sequence { ref_ })
    {
        ScopedWriteLock write (ref_->lock);
        sequence->readers_.erase (std::find (sequence->readers_.begin(), sequence->readers_.end(), this));
    }
    for (auto& x : sourceReaders_)
        delete x.second;
}

void ARARegionSequence::Reader::updateProperties (
    const ARA::PlugIn::PlaybackRegion& region,
    const ARA::PlugIn::AudioSource& source)
{
    numChannels = std::max (numChannels, (unsigned int) source.getChannelCount());

    if (sampleRate == 0.0)
        sampleRate = source.getSampleRate();
    jassert (sampleRate == source.getSampleRate());

    lengthInSamples = std::max (lengthInSamples, region.getEndInPlaybackSamples (sampleRate));
}

bool ARARegionSequence::Reader::readSamples (
    int** destSamples,
    int numDestChannels,
    int startOffsetInDestBuffer,
    int64 startSampleInFile,
    int numSamples)
{
    // Clear buffers
    for (int i = 0; i < numDestChannels; ++i)
        if (float* buf = (float*) destSamples[i])
            FloatVectorOperations::clear (buf + startOffsetInDestBuffer, numSamples);

    Ref::ScopedAccess sequence (ref_, true);
    if (! sequence)
        return false;

    if (buf_.getNumSamples() < numSamples || buf_.getNumChannels() < numDestChannels)
        buf_.setSize (numDestChannels, numSamples, false, false, true);

    const double start = (double) startSampleInFile / sampleRate;
    const double stop = (double) (startSampleInFile + (int64) numSamples) / sampleRate;

    // Fill in content from relevant regions
    for (ARA::PlugIn::PlaybackRegion* region : sequence->getPlaybackRegions())
    {
        if (region->getEndInPlaybackTime() <= start || region->getStartInPlaybackTime() >= stop)
            continue;

        const int64 regionStartSample = region->getStartInPlaybackSamples (sampleRate);

        AudioFormatReader* sourceReader = sourceReaders_[region->getAudioModification()->getAudioSource()];
        jassert (sourceReader != nullptr);

        const int64 startSampleInRegion = std::max ((int64) 0, startSampleInFile - regionStartSample);
        const int destOffest = (int) std::max ((int64) 0, regionStartSample - startSampleInFile);
        const int numRegionSamples = std::min (
                (int) (region->getDurationInPlaybackSamples (sampleRate) - startSampleInRegion),
                numSamples - destOffest);
        sourceReader->read (&buf_, 0, numRegionSamples, region->getStartInAudioModificationSamples() + startSampleInRegion, true, true);
        for (int chan_i = 0; chan_i < numDestChannels; ++chan_i)
            if (float* buf = (float*) destSamples[chan_i])
                FloatVectorOperations::add (
                    buf + startOffsetInDestBuffer + destOffest,
                    buf_.getReadPointer (chan_i), numRegionSamples);
    }

    return true;
}
