#ifndef _AL_SOURCE_H_
#define _AL_SOURCE_H_

#define MAX_SENDS                 4

#include "alMain.h"
#include "alu.h"
#include "hrtf.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ALbuffer;
struct ALsource;


typedef struct ALbufferlistitem {
    struct ALbuffer *buffer;
    struct ALbufferlistitem *volatile next;
    struct ALbufferlistitem *volatile prev;
} ALbufferlistitem;


typedef struct ALvoice {
    struct ALsource *volatile Source;

    /** Method to update mixing parameters. */
    //GIDEROS:hgy29: remove const to ALSource because ATOMIC_LOAD erroneously require non-const
    ALvoid (*Update)(struct ALvoice *self, struct ALsource *source, const ALCcontext *context);

    /** Current target parameters used for mixing. */
    ALint Step;

    ALboolean IsHrtf;

    ALuint Offset; /* Number of output samples mixed since starting. */

    alignas(16) ALfloat PrevSamples[MAX_INPUT_CHANNELS][MAX_PRE_SAMPLES];

    BsincState SincState;

    DirectParams Direct;
    SendParams Send[MAX_SENDS];
} ALvoice;


typedef struct ALsource {
    /** Source properties. */
    volatile ALfloat   Pitch;
    volatile ALfloat   Gain;
    volatile ALfloat   OuterGain;
    volatile ALfloat   MinGain;
    volatile ALfloat   MaxGain;
    volatile ALfloat   InnerAngle;
    volatile ALfloat   OuterAngle;
    volatile ALfloat   RefDistance;
    volatile ALfloat   MaxDistance;
    volatile ALfloat   RollOffFactor;
    aluVector Position;
    aluVector Velocity;
    aluVector Direction;
    volatile ALfloat   Orientation[2][3];
    volatile ALboolean HeadRelative;
    volatile ALboolean Looping;
    volatile enum DistanceModel DistanceModel;
    volatile ALboolean DirectChannels;

    volatile ALboolean DryGainHFAuto;
    volatile ALboolean WetGainAuto;
    volatile ALboolean WetGainHFAuto;
    volatile ALfloat   OuterGainHF;

    volatile ALfloat AirAbsorptionFactor;
    volatile ALfloat RoomRolloffFactor;
    volatile ALfloat DopplerFactor;

    volatile ALfloat Radius;

    /**
     * Last user-specified offset, and the offset type (bytes, samples, or
     * seconds).
     */
    ALdouble Offset;
    ALenum   OffsetType;

    /** Source type (static, streaming, or undetermined) */
    volatile ALint SourceType;

    /** Source state (initial, playing, paused, or stopped) */
    volatile ALenum state;
    ALenum new_state;

    /**
     * Source offset in samples, relative to the currently playing buffer, NOT
     * the whole queue, and the fractional (fixed-point) offset to the next
     * sample.
     */
    ALuint position;
    ALuint position_fraction;

    /** Source Buffer Queue info. */
    ATOMIC(ALbufferlistitem*) queue;
    ATOMIC(ALbufferlistitem*) current_buffer;
    RWLock queue_lock;

    /** Current buffer sample info. */
    ALuint NumChannels;
    ALuint SampleSize;

    /** Direct filter and auxiliary send info. */
    struct {
        ALfloat Gain;
        ALfloat GainHF;
        ALfloat HFReference;
        ALfloat GainLF;
        ALfloat LFReference;
    } Direct;
    struct {
        struct ALeffectslot *Slot;
        ALfloat Gain;
        ALfloat GainHF;
        ALfloat HFReference;
        ALfloat GainLF;
        ALfloat LFReference;
    } Send[MAX_SENDS];
    ALfloat Balance;

    /** Source needs to update its mixing parameters. */
    ATOMIC(ALenum) NeedsUpdate;

    /** Self ID */
    ALuint id;
} ALsource;

inline struct ALsource *LookupSource(ALCcontext *context, ALuint id)
{ return (struct ALsource*)LookupUIntMapKey(&context->SourceMap, id); }
inline struct ALsource *RemoveSource(ALCcontext *context, ALuint id)
{ return (struct ALsource*)RemoveUIntMapKey(&context->SourceMap, id); }

ALvoid SetSourceState(ALsource *Source, ALCcontext *Context, ALenum state);
ALboolean ApplyOffset(ALsource *Source);

ALvoid ReleaseALSources(ALCcontext *Context);

#ifdef __cplusplus
}
#endif

#endif
