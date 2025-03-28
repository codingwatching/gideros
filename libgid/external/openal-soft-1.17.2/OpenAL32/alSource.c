/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <float.h>

#include "AL/al.h"
#include "AL/alc.h"
#include "alMain.h"
#include "alError.h"
#include "alSource.h"
#include "alBuffer.h"
#include "alThunk.h"
#include "alAuxEffectSlot.h"

#include "backends/base.h"

#include "threads.h"


extern inline struct ALsource *LookupSource(ALCcontext *context, ALuint id);
extern inline struct ALsource *RemoveSource(ALCcontext *context, ALuint id);

static ALvoid InitSourceParams(ALsource *Source);
static ALint64 GetSourceSampleOffset(ALsource *Source);
static ALdouble GetSourceSecOffset(ALsource *Source);
static ALvoid GetSourceOffsets(ALsource *Source, ALenum name, ALdouble *offsets, ALdouble updateLen);
static ALboolean GetSampleOffset(ALsource *Source, ALuint *offset, ALuint *frac);

typedef enum SourceProp {
    srcPitch = AL_PITCH,
    srcGain = AL_GAIN,
    srcDirectGain = AL_DIRECT_GAIN,
    srcBalance = AL_BALANCE,
    srcMinGain = AL_MIN_GAIN,
    srcMaxGain = AL_MAX_GAIN,
    srcMaxDistance = AL_MAX_DISTANCE,
    srcRolloffFactor = AL_ROLLOFF_FACTOR,
    srcDopplerFactor = AL_DOPPLER_FACTOR,
    srcConeOuterGain = AL_CONE_OUTER_GAIN,
    srcSecOffset = AL_SEC_OFFSET,
    srcSampleOffset = AL_SAMPLE_OFFSET,
    srcByteOffset = AL_BYTE_OFFSET,
    srcConeInnerAngle = AL_CONE_INNER_ANGLE,
    srcConeOuterAngle = AL_CONE_OUTER_ANGLE,
    srcRefDistance = AL_REFERENCE_DISTANCE,

    srcPosition = AL_POSITION,
    srcVelocity = AL_VELOCITY,
    srcDirection = AL_DIRECTION,

    srcSourceRelative = AL_SOURCE_RELATIVE,
    srcLooping = AL_LOOPING,
    srcBuffer = AL_BUFFER,
    srcSourceState = AL_SOURCE_STATE,
    srcBuffersQueued = AL_BUFFERS_QUEUED,
    srcBuffersProcessed = AL_BUFFERS_PROCESSED,
    srcSourceType = AL_SOURCE_TYPE,

    /* ALC_EXT_EFX */
    srcConeOuterGainHF = AL_CONE_OUTER_GAINHF,
    srcAirAbsorptionFactor = AL_AIR_ABSORPTION_FACTOR,
    srcRoomRolloffFactor =  AL_ROOM_ROLLOFF_FACTOR,
    srcDirectFilterGainHFAuto = AL_DIRECT_FILTER_GAINHF_AUTO,
    srcAuxSendFilterGainAuto = AL_AUXILIARY_SEND_FILTER_GAIN_AUTO,
    srcAuxSendFilterGainHFAuto = AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO,
    srcDirectFilter = AL_DIRECT_FILTER,
    srcAuxSendFilter = AL_AUXILIARY_SEND_FILTER,

    /* AL_SOFT_direct_channels */
    srcDirectChannelsSOFT = AL_DIRECT_CHANNELS_SOFT,

    /* AL_EXT_source_distance_model */
    srcDistanceModel = AL_DISTANCE_MODEL,

    srcByteLengthSOFT = AL_BYTE_LENGTH_SOFT,
    srcSampleLengthSOFT = AL_SAMPLE_LENGTH_SOFT,
    srcSecLengthSOFT = AL_SEC_LENGTH_SOFT,

    /* AL_SOFT_buffer_sub_data / AL_SOFT_buffer_samples */
    srcSampleRWOffsetsSOFT = AL_SAMPLE_RW_OFFSETS_SOFT,
    srcByteRWOffsetsSOFT = AL_BYTE_RW_OFFSETS_SOFT,

    /* AL_SOFT_source_latency */
    srcSampleOffsetLatencySOFT = AL_SAMPLE_OFFSET_LATENCY_SOFT,
    srcSecOffsetLatencySOFT = AL_SEC_OFFSET_LATENCY_SOFT,

    /* AL_EXT_BFORMAT */
    srcOrientation = AL_ORIENTATION,
} SourceProp;

static ALboolean SetSourcefv(ALsource *Source, ALCcontext *Context, SourceProp prop, const ALfloat *values);
static ALboolean SetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop, const ALint *values);
static ALboolean SetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop, const ALint64SOFT *values);

static ALboolean GetSourcedv(ALsource *Source, ALCcontext *Context, SourceProp prop, ALdouble *values);
static ALboolean GetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop, ALint *values);
static ALboolean GetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop, ALint64 *values);

static ALint FloatValsByProp(ALenum prop)
{
    if(prop != (ALenum)((SourceProp)prop))
        return 0;
    switch((SourceProp)prop)
    {
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_MAX_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_DOPPLER_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_REFERENCE_DISTANCE:
        case AL_CONE_OUTER_GAINHF:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
        case AL_DISTANCE_MODEL:
        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_SOURCE_STATE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_SOURCE_TYPE:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SEC_LENGTH_SOFT:
            return 1;

        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
            return 2;

        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
            return 3;

        case AL_ORIENTATION:
            return 6;

        case AL_SEC_OFFSET_LATENCY_SOFT:
            break; /* Double only */

        case AL_BUFFER:
        case AL_DIRECT_FILTER:
        case AL_AUXILIARY_SEND_FILTER:
            break; /* i/i64 only */
        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            break; /* i64 only */
    }
    return 0;
}
static ALint DoubleValsByProp(ALenum prop)
{
    if(prop != (ALenum)((SourceProp)prop))
        return 0;
    switch((SourceProp)prop)
    {
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_MAX_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_DOPPLER_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_REFERENCE_DISTANCE:
        case AL_CONE_OUTER_GAINHF:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
        case AL_DISTANCE_MODEL:
        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_SOURCE_STATE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_SOURCE_TYPE:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SEC_LENGTH_SOFT:
            return 1;

        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
        case AL_SEC_OFFSET_LATENCY_SOFT:
            return 2;

        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
            return 3;

        case AL_ORIENTATION:
            return 6;

        case AL_BUFFER:
        case AL_DIRECT_FILTER:
        case AL_AUXILIARY_SEND_FILTER:
            break; /* i/i64 only */
        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            break; /* i64 only */
    }
    return 0;
}

static ALint IntValsByProp(ALenum prop)
{
    if(prop != (ALenum)((SourceProp)prop))
        return 0;
    switch((SourceProp)prop)
    {
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_MAX_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_DOPPLER_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_REFERENCE_DISTANCE:
        case AL_CONE_OUTER_GAINHF:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
        case AL_DISTANCE_MODEL:
        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_BUFFER:
        case AL_SOURCE_STATE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_SOURCE_TYPE:
        case AL_DIRECT_FILTER:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SEC_LENGTH_SOFT:
            return 1;

        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
            return 2;

        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
        case AL_AUXILIARY_SEND_FILTER:
            return 3;

        case AL_ORIENTATION:
            return 6;

        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            break; /* i64 only */
        case AL_SEC_OFFSET_LATENCY_SOFT:
            break; /* Double only */
    }
    return 0;
}
static ALint Int64ValsByProp(ALenum prop)
{
    if(prop != (ALenum)((SourceProp)prop))
        return 0;
    switch((SourceProp)prop)
    {
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_MAX_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_DOPPLER_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_REFERENCE_DISTANCE:
        case AL_CONE_OUTER_GAINHF:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
        case AL_DISTANCE_MODEL:
        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_BUFFER:
        case AL_SOURCE_STATE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_SOURCE_TYPE:
        case AL_DIRECT_FILTER:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SEC_LENGTH_SOFT:
            return 1;

        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            return 2;

        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
        case AL_AUXILIARY_SEND_FILTER:
            return 3;

        case AL_ORIENTATION:
            return 6;

        case AL_SEC_OFFSET_LATENCY_SOFT:
            break; /* Double only */
    }
    return 0;
}


#define CHECKVAL(x) do {                                                      \
    if(!(x))                                                                  \
        SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_VALUE, AL_FALSE);      \
} while(0)

static ALboolean SetSourcefv(ALsource *Source, ALCcontext *Context, SourceProp prop, const ALfloat *values)
{
    ALint ival;

    switch(prop)
    {
        case AL_BYTE_RW_OFFSETS_SOFT:
        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SEC_LENGTH_SOFT:
        case AL_SEC_OFFSET_LATENCY_SOFT:
            /* Query only */
            SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_OPERATION, AL_FALSE);

        case AL_PITCH:
            CHECKVAL(*values >= 0.0f);

            Source->Pitch = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_CONE_INNER_ANGLE:
            CHECKVAL(*values >= 0.0f && *values <= 360.0f);

            Source->InnerAngle = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_CONE_OUTER_ANGLE:
            CHECKVAL(*values >= 0.0f && *values <= 360.0f);

            Source->OuterAngle = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_GAIN:
            CHECKVAL(*values >= 0.0f);

            Source->Gain = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_BALANCE:
            CHECKVAL(*values >= -1.0f && *values <= 1.0f);

            Source->Balance = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_DIRECT_GAIN:
            CHECKVAL(*values >= 0.0f);

            Source->Direct.Gain = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_MAX_DISTANCE:
            CHECKVAL(*values >= 0.0f);

            Source->MaxDistance = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_ROLLOFF_FACTOR:
            CHECKVAL(*values >= 0.0f);

            Source->RollOffFactor = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_REFERENCE_DISTANCE:
            CHECKVAL(*values >= 0.0f);

            Source->RefDistance = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_MIN_GAIN:
            CHECKVAL(*values >= 0.0f && *values <= 1.0f);

            Source->MinGain = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_MAX_GAIN:
            CHECKVAL(*values >= 0.0f && *values <= 1.0f);

            Source->MaxGain = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_CONE_OUTER_GAIN:
            CHECKVAL(*values >= 0.0f && *values <= 1.0f);

            Source->OuterGain = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_CONE_OUTER_GAINHF:
            CHECKVAL(*values >= 0.0f && *values <= 1.0f);

            Source->OuterGainHF = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_AIR_ABSORPTION_FACTOR:
            CHECKVAL(*values >= 0.0f && *values <= 10.0f);

            Source->AirAbsorptionFactor = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_ROOM_ROLLOFF_FACTOR:
            CHECKVAL(*values >= 0.0f && *values <= 10.0f);

            Source->RoomRolloffFactor = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_DOPPLER_FACTOR:
            CHECKVAL(*values >= 0.0f && *values <= 1.0f);

            Source->DopplerFactor = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
            CHECKVAL(*values >= 0.0f);

            LockContext(Context);
            Source->OffsetType = prop;
            Source->Offset = *values;

            if((Source->state == AL_PLAYING || Source->state == AL_PAUSED) &&
               !Context->DeferUpdates)
            {
                WriteLock(&Source->queue_lock);
                if(ApplyOffset(Source) == AL_FALSE)
                {
                    WriteUnlock(&Source->queue_lock);
                    UnlockContext(Context);
                    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_VALUE, AL_FALSE);
                }
                WriteUnlock(&Source->queue_lock);
            }
            UnlockContext(Context);
            return AL_TRUE;


        case AL_POSITION:
            CHECKVAL(isfinite(values[0]) && isfinite(values[1]) && isfinite(values[2]));

            LockContext(Context);
            aluVectorSet(&Source->Position, values[0], values[1], values[2], 1.0f);
            UnlockContext(Context);
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_VELOCITY:
            CHECKVAL(isfinite(values[0]) && isfinite(values[1]) && isfinite(values[2]));

            LockContext(Context);
            aluVectorSet(&Source->Velocity, values[0], values[1], values[2], 0.0f);
            UnlockContext(Context);
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_DIRECTION:
            CHECKVAL(isfinite(values[0]) && isfinite(values[1]) && isfinite(values[2]));

            LockContext(Context);
            aluVectorSet(&Source->Direction, values[0], values[1], values[2], 0.0f);
            UnlockContext(Context);
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_ORIENTATION:
            CHECKVAL(isfinite(values[0]) && isfinite(values[1]) && isfinite(values[2]) &&
                     isfinite(values[3]) && isfinite(values[4]) && isfinite(values[5]));

            LockContext(Context);
            Source->Orientation[0][0] = values[0];
            Source->Orientation[0][1] = values[1];
            Source->Orientation[0][2] = values[2];
            Source->Orientation[1][0] = values[3];
            Source->Orientation[1][1] = values[4];
            Source->Orientation[1][2] = values[5];
            UnlockContext(Context);
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;


        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_SOURCE_STATE:
        case AL_SOURCE_TYPE:
        case AL_DISTANCE_MODEL:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
            ival = (ALint)values[0];
            return SetSourceiv(Source, Context, prop, &ival);

        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
            ival = (ALint)((ALuint)values[0]);
            return SetSourceiv(Source, Context, prop, &ival);

        case AL_BUFFER:
        case AL_DIRECT_FILTER:
        case AL_AUXILIARY_SEND_FILTER:
        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_ENUM, AL_FALSE);
}

static ALboolean SetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop, const ALint *values)
{
    ALCdevice *device = Context->Device;
    ALbuffer  *buffer = NULL;
    ALfilter  *filter = NULL;
    ALeffectslot *slot = NULL;
    ALbufferlistitem *oldlist;
    ALbufferlistitem *newlist;
    ALfloat fvals[6];

    switch(prop)
    {
        case AL_SOURCE_STATE:
        case AL_SOURCE_TYPE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SEC_LENGTH_SOFT:
            /* Query only */
            SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_OPERATION, AL_FALSE);

        case AL_SOURCE_RELATIVE:
            CHECKVAL(*values == AL_FALSE || *values == AL_TRUE);

            Source->HeadRelative = (ALboolean)*values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_LOOPING:
            CHECKVAL(*values == AL_FALSE || *values == AL_TRUE);

            Source->Looping = (ALboolean)*values;
            return AL_TRUE;

        case AL_BUFFER:
            CHECKVAL(*values == 0 || (buffer=LookupBuffer(device, *values)) != NULL);

            WriteLock(&Source->queue_lock);
            if(!(Source->state == AL_STOPPED || Source->state == AL_INITIAL))
            {
                WriteUnlock(&Source->queue_lock);
                SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_OPERATION, AL_FALSE);
            }

            if(buffer != NULL)
            {
                /* Add the selected buffer to a one-item queue */
                newlist = malloc(sizeof(ALbufferlistitem));
                newlist->buffer = buffer;
                newlist->next = NULL;
                newlist->prev = NULL;
                IncrementRef(&buffer->ref);

                /* Source is now Static */
                Source->SourceType = AL_STATIC;

                ReadLock(&buffer->lock);
                Source->NumChannels = ChannelsFromFmt(buffer->FmtChannels);
                Source->SampleSize  = BytesFromFmt(buffer->FmtType);
                ReadUnlock(&buffer->lock);
            }
            else
            {
                /* Source is now Undetermined */
                Source->SourceType = AL_UNDETERMINED;
                newlist = NULL;
            }
            oldlist = ATOMIC_EXCHANGE(ALbufferlistitem*, &Source->queue, newlist);
            ATOMIC_STORE(&Source->current_buffer, newlist);
            WriteUnlock(&Source->queue_lock);

            /* Delete all elements in the previous queue */
            while(oldlist != NULL)
            {
                ALbufferlistitem *temp = oldlist;
                oldlist = temp->next;

                if(temp->buffer)
                    DecrementRef(&temp->buffer->ref);
                free(temp);
            }
            return AL_TRUE;

        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
            CHECKVAL(*values >= 0);

            LockContext(Context);
            Source->OffsetType = prop;
            Source->Offset = *values;

            if((Source->state == AL_PLAYING || Source->state == AL_PAUSED) &&
                !Context->DeferUpdates)
            {
                WriteLock(&Source->queue_lock);
                if(ApplyOffset(Source) == AL_FALSE)
                {
                    WriteUnlock(&Source->queue_lock);
                    UnlockContext(Context);
                    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_VALUE, AL_FALSE);
                }
                WriteUnlock(&Source->queue_lock);
            }
            UnlockContext(Context);
            return AL_TRUE;

        case AL_DIRECT_FILTER:
            CHECKVAL(*values == 0 || (filter=LookupFilter(device, *values)) != NULL);

            LockContext(Context);
            if(!filter)
            {
                Source->Direct.Gain = 1.0f;
                Source->Direct.GainHF = 1.0f;
                Source->Direct.HFReference = LOWPASSFREQREF;
                Source->Direct.GainLF = 1.0f;
                Source->Direct.LFReference = HIGHPASSFREQREF;
            }
            else
            {
                Source->Direct.Gain = filter->Gain;
                Source->Direct.GainHF = filter->GainHF;
                Source->Direct.HFReference = filter->HFReference;
                Source->Direct.GainLF = filter->GainLF;
                Source->Direct.LFReference = filter->LFReference;
            }
            UnlockContext(Context);
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_DIRECT_FILTER_GAINHF_AUTO:
            CHECKVAL(*values == AL_FALSE || *values == AL_TRUE);

            Source->DryGainHFAuto = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
            CHECKVAL(*values == AL_FALSE || *values == AL_TRUE);

            Source->WetGainAuto = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
            CHECKVAL(*values == AL_FALSE || *values == AL_TRUE);

            Source->WetGainHFAuto = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_DIRECT_CHANNELS_SOFT:
            CHECKVAL(*values == AL_FALSE || *values == AL_TRUE);

            Source->DirectChannels = *values;
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;

        case AL_DISTANCE_MODEL:
            CHECKVAL(*values == AL_NONE ||
                     *values == AL_INVERSE_DISTANCE ||
                     *values == AL_INVERSE_DISTANCE_CLAMPED ||
                     *values == AL_LINEAR_DISTANCE ||
                     *values == AL_LINEAR_DISTANCE_CLAMPED ||
                     *values == AL_EXPONENT_DISTANCE ||
                     *values == AL_EXPONENT_DISTANCE_CLAMPED);

            Source->DistanceModel = *values;
            if(Context->SourceDistanceModel)
                ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;


        case AL_AUXILIARY_SEND_FILTER:
            LockContext(Context);
            if(!((ALuint)values[1] < device->NumAuxSends &&
                 (values[0] == 0 || (slot=LookupEffectSlot(Context, values[0])) != NULL) &&
                 (values[2] == 0 || (filter=LookupFilter(device, values[2])) != NULL)))
            {
                UnlockContext(Context);
                SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_VALUE, AL_FALSE);
            }

            /* Add refcount on the new slot, and release the previous slot */
            if(slot) IncrementRef(&slot->ref);
            slot = ExchangePtr((XchgPtr*)&Source->Send[values[1]].Slot, slot);
            if(slot) DecrementRef(&slot->ref);

            if(!filter)
            {
                /* Disable filter */
                Source->Send[values[1]].Gain = 1.0f;
                Source->Send[values[1]].GainHF = 1.0f;
                Source->Send[values[1]].HFReference = LOWPASSFREQREF;
                Source->Send[values[1]].GainLF = 1.0f;
                Source->Send[values[1]].LFReference = HIGHPASSFREQREF;
            }
            else
            {
                Source->Send[values[1]].Gain = filter->Gain;
                Source->Send[values[1]].GainHF = filter->GainHF;
                Source->Send[values[1]].HFReference = filter->HFReference;
                Source->Send[values[1]].GainLF = filter->GainLF;
                Source->Send[values[1]].LFReference = filter->LFReference;
            }
            UnlockContext(Context);
            ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
            return AL_TRUE;


        /* 1x float */
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_REFERENCE_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_MAX_DISTANCE:
        case AL_DOPPLER_FACTOR:
        case AL_CONE_OUTER_GAINHF:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
            fvals[0] = (ALfloat)*values;
            return SetSourcefv(Source, Context, (int)prop, fvals);

        /* 3x float */
        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
            fvals[0] = (ALfloat)values[0];
            fvals[1] = (ALfloat)values[1];
            fvals[2] = (ALfloat)values[2];
            return SetSourcefv(Source, Context, (int)prop, fvals);

        /* 6x float */
        case AL_ORIENTATION:
            fvals[0] = (ALfloat)values[0];
            fvals[1] = (ALfloat)values[1];
            fvals[2] = (ALfloat)values[2];
            fvals[3] = (ALfloat)values[3];
            fvals[4] = (ALfloat)values[4];
            fvals[5] = (ALfloat)values[5];
            return SetSourcefv(Source, Context, (int)prop, fvals);

        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
        case AL_SEC_OFFSET_LATENCY_SOFT:
            break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_ENUM, AL_FALSE);
}

static ALboolean SetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop, const ALint64SOFT *values)
{
    ALfloat fvals[6];
    ALint   ivals[3];

    switch(prop)
    {
        case AL_SOURCE_TYPE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_SOURCE_STATE:
        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SEC_LENGTH_SOFT:
            /* Query only */
            SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_OPERATION, AL_FALSE);


        /* 1x int */
        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
        case AL_DISTANCE_MODEL:
            CHECKVAL(*values <= INT_MAX && *values >= INT_MIN);

            ivals[0] = (ALint)*values;
            return SetSourceiv(Source, Context, (int)prop, ivals);

        /* 1x uint */
        case AL_BUFFER:
        case AL_DIRECT_FILTER:
            CHECKVAL(*values <= UINT_MAX && *values >= 0);

            ivals[0] = (ALuint)*values;
            return SetSourceiv(Source, Context, (int)prop, ivals);

        /* 3x uint */
        case AL_AUXILIARY_SEND_FILTER:
            CHECKVAL(values[0] <= UINT_MAX && values[0] >= 0 &&
                     values[1] <= UINT_MAX && values[1] >= 0 &&
                     values[2] <= UINT_MAX && values[2] >= 0);

            ivals[0] = (ALuint)values[0];
            ivals[1] = (ALuint)values[1];
            ivals[2] = (ALuint)values[2];
            return SetSourceiv(Source, Context, (int)prop, ivals);

        /* 1x float */
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_REFERENCE_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_MAX_DISTANCE:
        case AL_DOPPLER_FACTOR:
        case AL_CONE_OUTER_GAINHF:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
            fvals[0] = (ALfloat)*values;
            return SetSourcefv(Source, Context, (int)prop, fvals);

        /* 3x float */
        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
            fvals[0] = (ALfloat)values[0];
            fvals[1] = (ALfloat)values[1];
            fvals[2] = (ALfloat)values[2];
            return SetSourcefv(Source, Context, (int)prop, fvals);

        /* 6x float */
        case AL_ORIENTATION:
            fvals[0] = (ALfloat)values[0];
            fvals[1] = (ALfloat)values[1];
            fvals[2] = (ALfloat)values[2];
            fvals[3] = (ALfloat)values[3];
            fvals[4] = (ALfloat)values[4];
            fvals[5] = (ALfloat)values[5];
            return SetSourcefv(Source, Context, (int)prop, fvals);

        case AL_SEC_OFFSET_LATENCY_SOFT:
            break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_ENUM, AL_FALSE);
}

#undef CHECKVAL


static ALboolean GetSourcedv(ALsource *Source, ALCcontext *Context, SourceProp prop, ALdouble *values)
{
    ALCdevice *device = Context->Device;
    ALbufferlistitem *BufferList;
    ALdouble offsets[2];
    ALdouble updateLen;
    ALint ivals[3];
    ALboolean err;

    switch(prop)
    {
        case AL_GAIN:
            *values = Source->Gain;
            return AL_TRUE;

        case AL_DIRECT_GAIN:
            *values = Source->Direct.Gain;
            return AL_TRUE;

        case AL_BALANCE:
            *values = Source->Balance;
            return AL_TRUE;

        case AL_PITCH:
            *values = Source->Pitch;
            return AL_TRUE;

        case AL_MAX_DISTANCE:
            *values = Source->MaxDistance;
            return AL_TRUE;

        case AL_ROLLOFF_FACTOR:
            *values = Source->RollOffFactor;
            return AL_TRUE;

        case AL_REFERENCE_DISTANCE:
            *values = Source->RefDistance;
            return AL_TRUE;

        case AL_CONE_INNER_ANGLE:
            *values = Source->InnerAngle;
            return AL_TRUE;

        case AL_CONE_OUTER_ANGLE:
            *values = Source->OuterAngle;
            return AL_TRUE;

        case AL_MIN_GAIN:
            *values = Source->MinGain;
            return AL_TRUE;

        case AL_MAX_GAIN:
            *values = Source->MaxGain;
            return AL_TRUE;

        case AL_CONE_OUTER_GAIN:
            *values = Source->OuterGain;
            return AL_TRUE;

        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
            LockContext(Context);
            GetSourceOffsets(Source, prop, offsets, 0.0);
            UnlockContext(Context);
            *values = offsets[0];
            return AL_TRUE;

        case AL_CONE_OUTER_GAINHF:
            *values = Source->OuterGainHF;
            return AL_TRUE;

        case AL_AIR_ABSORPTION_FACTOR:
            *values = Source->AirAbsorptionFactor;
            return AL_TRUE;

        case AL_ROOM_ROLLOFF_FACTOR:
            *values = Source->RoomRolloffFactor;
            return AL_TRUE;

        case AL_DOPPLER_FACTOR:
            *values = Source->DopplerFactor;
            return AL_TRUE;

        case AL_SEC_LENGTH_SOFT:
            ReadLock(&Source->queue_lock);
            if(!(BufferList=ATOMIC_LOAD(&Source->queue)))
                *values = 0;
            else
            {
                ALint length = 0;
                ALsizei freq = 1;
                do {
                    ALbuffer *buffer = BufferList->buffer;
                    if(buffer && buffer->SampleLen > 0)
                    {
                        freq = buffer->Frequency;
                        length += buffer->SampleLen;
                    }
                } while((BufferList=BufferList->next) != NULL);
                *values = (ALdouble)length / (ALdouble)freq;
            }
            ReadUnlock(&Source->queue_lock);
            return AL_TRUE;

        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
            LockContext(Context);
            updateLen = (ALdouble)device->UpdateSize / device->Frequency;
            GetSourceOffsets(Source, prop, values, updateLen);
            UnlockContext(Context);
            return AL_TRUE;

        case AL_SEC_OFFSET_LATENCY_SOFT:
            LockContext(Context);
            values[0] = GetSourceSecOffset(Source);
            values[1] = (ALdouble)(V0(device->Backend,getLatency)()) /
                        1000000000.0;
            UnlockContext(Context);
            return AL_TRUE;

        case AL_POSITION:
            LockContext(Context);
            values[0] = Source->Position.v[0];
            values[1] = Source->Position.v[1];
            values[2] = Source->Position.v[2];
            UnlockContext(Context);
            return AL_TRUE;

        case AL_VELOCITY:
            LockContext(Context);
            values[0] = Source->Velocity.v[0];
            values[1] = Source->Velocity.v[1];
            values[2] = Source->Velocity.v[2];
            UnlockContext(Context);
            return AL_TRUE;

        case AL_DIRECTION:
            LockContext(Context);
            values[0] = Source->Direction.v[0];
            values[1] = Source->Direction.v[1];
            values[2] = Source->Direction.v[2];
            UnlockContext(Context);
            return AL_TRUE;

        case AL_ORIENTATION:
            LockContext(Context);
            values[0] = Source->Orientation[0][0];
            values[1] = Source->Orientation[0][1];
            values[2] = Source->Orientation[0][2];
            values[3] = Source->Orientation[1][0];
            values[4] = Source->Orientation[1][1];
            values[5] = Source->Orientation[1][2];
            UnlockContext(Context);
            return AL_TRUE;

        /* 1x int */
        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_SOURCE_STATE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_SOURCE_TYPE:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_DISTANCE_MODEL:
            if((err=GetSourceiv(Source, Context, (int)prop, ivals)) != AL_FALSE)
                *values = (ALdouble)ivals[0];
            return err;

        case AL_BUFFER:
        case AL_DIRECT_FILTER:
        case AL_AUXILIARY_SEND_FILTER:
        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_ENUM, AL_FALSE);
}

static ALboolean GetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop, ALint *values)
{
    ALbufferlistitem *BufferList;
    ALdouble dvals[6];
    ALboolean err;

    switch(prop)
    {
        case AL_SOURCE_RELATIVE:
            *values = Source->HeadRelative;
            return AL_TRUE;

        case AL_LOOPING:
            *values = Source->Looping;
            return AL_TRUE;

        case AL_BUFFER:
            ReadLock(&Source->queue_lock);
            BufferList = (Source->SourceType == AL_STATIC) ? ATOMIC_LOAD(&Source->queue) :
                                                             ATOMIC_LOAD(&Source->current_buffer);
            *values = (BufferList && BufferList->buffer) ? BufferList->buffer->id : 0;
            ReadUnlock(&Source->queue_lock);
            return AL_TRUE;

        case AL_SOURCE_STATE:
            *values = Source->state;
            return AL_TRUE;

        case AL_BYTE_LENGTH_SOFT:
            ReadLock(&Source->queue_lock);
            if(!(BufferList=ATOMIC_LOAD(&Source->queue)))
                *values = 0;
            else
            {
                ALint length = 0;
                do {
                    ALbuffer *buffer = BufferList->buffer;
                    if(buffer && buffer->SampleLen > 0)
                    {
                        ALuint byte_align, sample_align;
                        if(buffer->OriginalType == UserFmtIMA4)
                        {
                            ALsizei align = (buffer->OriginalAlign-1)/2 + 4;
                            byte_align = align * ChannelsFromFmt(buffer->FmtChannels);
                            sample_align = buffer->OriginalAlign;
                        }
                        else if(buffer->OriginalType == UserFmtMSADPCM)
                        {
                            ALsizei align = (buffer->OriginalAlign-2)/2 + 7;
                            byte_align = align * ChannelsFromFmt(buffer->FmtChannels);
                            sample_align = buffer->OriginalAlign;
                        }
                        else
                        {
                            ALsizei align = buffer->OriginalAlign;
                            byte_align = align * ChannelsFromFmt(buffer->FmtChannels);
                            sample_align = buffer->OriginalAlign;
                        }

                        length += buffer->SampleLen / sample_align * byte_align;
                    }
                } while((BufferList=BufferList->next) != NULL);
                *values = length;
            }
            ReadUnlock(&Source->queue_lock);
            return AL_TRUE;

        case AL_SAMPLE_LENGTH_SOFT:
            ReadLock(&Source->queue_lock);
            if(!(BufferList=ATOMIC_LOAD(&Source->queue)))
                *values = 0;
            else
            {
                ALint length = 0;
                do {
                    ALbuffer *buffer = BufferList->buffer;
                    if(buffer) length += buffer->SampleLen;
                } while((BufferList=BufferList->next) != NULL);
                *values = length;
            }
            ReadUnlock(&Source->queue_lock);
            return AL_TRUE;

        case AL_BUFFERS_QUEUED:
            ReadLock(&Source->queue_lock);
            if(!(BufferList=ATOMIC_LOAD(&Source->queue)))
                *values = 0;
            else
            {
                ALsizei count = 0;
                do {
                    ++count;
                } while((BufferList=BufferList->next) != NULL);
                *values = count;
            }
            ReadUnlock(&Source->queue_lock);
            return AL_TRUE;

        case AL_BUFFERS_PROCESSED:
            ReadLock(&Source->queue_lock);
            if(Source->Looping || Source->SourceType != AL_STREAMING)
            {
                /* Buffers on a looping source are in a perpetual state of
                 * PENDING, so don't report any as PROCESSED */
                *values = 0;
            }
            else
            {
                const ALbufferlistitem *BufferList = ATOMIC_LOAD(&Source->queue);
                const ALbufferlistitem *Current = ATOMIC_LOAD(&Source->current_buffer);
                ALsizei played = 0;
                while(BufferList && BufferList != Current)
                {
                    played++;
                    BufferList = BufferList->next;
                }
                *values = played;
            }
            ReadUnlock(&Source->queue_lock);
            return AL_TRUE;

        case AL_SOURCE_TYPE:
            *values = Source->SourceType;
            return AL_TRUE;

        case AL_DIRECT_FILTER_GAINHF_AUTO:
            *values = Source->DryGainHFAuto;
            return AL_TRUE;

        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
            *values = Source->WetGainAuto;
            return AL_TRUE;

        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
            *values = Source->WetGainHFAuto;
            return AL_TRUE;

        case AL_DIRECT_CHANNELS_SOFT:
            *values = Source->DirectChannels;
            return AL_TRUE;

        case AL_DISTANCE_MODEL:
            *values = Source->DistanceModel;
            return AL_TRUE;

        /* 1x float/double */
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_REFERENCE_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_MAX_DISTANCE:
        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
        case AL_DOPPLER_FACTOR:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
        case AL_CONE_OUTER_GAINHF:
        case AL_SEC_LENGTH_SOFT:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
                *values = (ALint)dvals[0];
            return err;

        /* 2x float/double */
        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
            {
                values[0] = (ALint)dvals[0];
                values[1] = (ALint)dvals[1];
            }
            return err;

        /* 3x float/double */
        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
            {
                values[0] = (ALint)dvals[0];
                values[1] = (ALint)dvals[1];
                values[2] = (ALint)dvals[2];
            }
            return err;

        /* 6x float/double */
        case AL_ORIENTATION:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
            {
                values[0] = (ALint)dvals[0];
                values[1] = (ALint)dvals[1];
                values[2] = (ALint)dvals[2];
                values[3] = (ALint)dvals[3];
                values[4] = (ALint)dvals[4];
                values[5] = (ALint)dvals[5];
            }
            return err;

        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            break; /* i64 only */
        case AL_SEC_OFFSET_LATENCY_SOFT:
            break; /* Double only */

        case AL_DIRECT_FILTER:
        case AL_AUXILIARY_SEND_FILTER:
            break; /* ??? */
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_ENUM, AL_FALSE);
}

static ALboolean GetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop, ALint64 *values)
{
    ALCdevice *device = Context->Device;
    ALdouble dvals[6];
    ALint ivals[3];
    ALboolean err;

    switch(prop)
    {
        case AL_SAMPLE_OFFSET_LATENCY_SOFT:
            LockContext(Context);
            values[0] = GetSourceSampleOffset(Source);
            values[1] = V0(device->Backend,getLatency)();
            UnlockContext(Context);
            return AL_TRUE;

        /* 1x float/double */
        case AL_CONE_INNER_ANGLE:
        case AL_CONE_OUTER_ANGLE:
        case AL_PITCH:
        case AL_GAIN:
        case AL_DIRECT_GAIN:
        case AL_BALANCE:
        case AL_MIN_GAIN:
        case AL_MAX_GAIN:
        case AL_REFERENCE_DISTANCE:
        case AL_ROLLOFF_FACTOR:
        case AL_CONE_OUTER_GAIN:
        case AL_MAX_DISTANCE:
        case AL_SEC_OFFSET:
        case AL_SAMPLE_OFFSET:
        case AL_BYTE_OFFSET:
        case AL_DOPPLER_FACTOR:
        case AL_AIR_ABSORPTION_FACTOR:
        case AL_ROOM_ROLLOFF_FACTOR:
        case AL_CONE_OUTER_GAINHF:
        case AL_SEC_LENGTH_SOFT:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
                *values = (ALint64)dvals[0];
            return err;

        /* 2x float/double */
        case AL_SAMPLE_RW_OFFSETS_SOFT:
        case AL_BYTE_RW_OFFSETS_SOFT:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
            {
                values[0] = (ALint64)dvals[0];
                values[1] = (ALint64)dvals[1];
            }
            return err;

        /* 3x float/double */
        case AL_POSITION:
        case AL_VELOCITY:
        case AL_DIRECTION:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
            {
                values[0] = (ALint64)dvals[0];
                values[1] = (ALint64)dvals[1];
                values[2] = (ALint64)dvals[2];
            }
            return err;

        /* 6x float/double */
        case AL_ORIENTATION:
            if((err=GetSourcedv(Source, Context, prop, dvals)) != AL_FALSE)
            {
                values[0] = (ALint64)dvals[0];
                values[1] = (ALint64)dvals[1];
                values[2] = (ALint64)dvals[2];
                values[3] = (ALint64)dvals[3];
                values[4] = (ALint64)dvals[4];
                values[5] = (ALint64)dvals[5];
            }
            return err;

        /* 1x int */
        case AL_SOURCE_RELATIVE:
        case AL_LOOPING:
        case AL_SOURCE_STATE:
        case AL_BUFFERS_QUEUED:
        case AL_BUFFERS_PROCESSED:
        case AL_BYTE_LENGTH_SOFT:
        case AL_SAMPLE_LENGTH_SOFT:
        case AL_SOURCE_TYPE:
        case AL_DIRECT_FILTER_GAINHF_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        case AL_DIRECT_CHANNELS_SOFT:
        case AL_DISTANCE_MODEL:
            if((err=GetSourceiv(Source, Context, prop, ivals)) != AL_FALSE)
                *values = ivals[0];
            return err;

        /* 1x uint */
        case AL_BUFFER:
        case AL_DIRECT_FILTER:
            if((err=GetSourceiv(Source, Context, prop, ivals)) != AL_FALSE)
                *values = (ALuint)ivals[0];
            return err;

        /* 3x uint */
        case AL_AUXILIARY_SEND_FILTER:
            if((err=GetSourceiv(Source, Context, prop, ivals)) != AL_FALSE)
            {
                values[0] = (ALuint)ivals[0];
                values[1] = (ALuint)ivals[1];
                values[2] = (ALuint)ivals[2];
            }
            return err;

        case AL_SEC_OFFSET_LATENCY_SOFT:
            break; /* Double only */
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    SET_ERROR_AND_RETURN_VALUE(Context, AL_INVALID_ENUM, AL_FALSE);
}


AL_API ALvoid AL_APIENTRY alGenSources(ALsizei n, ALuint *sources)
{
    ALCcontext *context;
    ALsizei cur = 0;
    ALenum err;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
    for(cur = 0;cur < n;cur++)
    {
        ALsource *source = al_calloc(16, sizeof(ALsource));
        if(!source)
        {
            alDeleteSources(cur, sources);
            SET_ERROR_AND_GOTO(context, AL_OUT_OF_MEMORY, done);
        }
        InitSourceParams(source);

        err = NewThunkEntry(&source->id);
        if(err == AL_NO_ERROR)
            err = InsertUIntMapEntry(&context->SourceMap, source->id, source);
        if(err != AL_NO_ERROR)
        {
            FreeThunkEntry(source->id);
            memset(source, 0, sizeof(ALsource));
            al_free(source);

            alDeleteSources(cur, sources);
            SET_ERROR_AND_GOTO(context, err, done);
        }

        sources[cur] = source->id;
    }

done:
    ALCcontext_DecRef(context);
}


AL_API ALvoid AL_APIENTRY alDeleteSources(ALsizei n, const ALuint *sources)
{
    ALCcontext *context;
    ALbufferlistitem *BufferList;
    ALsource *Source;
    ALsizei i, j;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);

    /* Check that all Sources are valid */
    for(i = 0;i < n;i++)
    {
        if(LookupSource(context, sources[i]) == NULL)
            SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    }
    for(i = 0;i < n;i++)
    {
        ALvoice *voice, *voice_end;

        if((Source=RemoveSource(context, sources[i])) == NULL)
            continue;
        FreeThunkEntry(Source->id);

        LockContext(context);
        voice = context->Voices;
        voice_end = voice + context->VoiceCount;
        while(voice != voice_end)
        {
            ALsource *old = Source;
            if(COMPARE_EXCHANGE(&voice->Source, &old, NULL))
                break;
            voice++;
        }
        UnlockContext(context);

        BufferList = ATOMIC_EXCHANGE(ALbufferlistitem*, &Source->queue, NULL);
        while(BufferList != NULL)
        {
            ALbufferlistitem *next = BufferList->next;
            if(BufferList->buffer != NULL)
                DecrementRef(&BufferList->buffer->ref);
            free(BufferList);
            BufferList = next;
        }

        for(j = 0;j < MAX_SENDS;++j)
        {
            if(Source->Send[j].Slot)
                DecrementRef(&Source->Send[j].Slot->ref);
            Source->Send[j].Slot = NULL;
        }

        memset(Source, 0, sizeof(*Source));
        al_free(Source);
    }

done:
    ALCcontext_DecRef(context);
}


AL_API ALboolean AL_APIENTRY alIsSource(ALuint source)
{
    ALCcontext *context;
    ALboolean ret;

    context = GetContextRef();
    if(!context) return AL_FALSE;

    ret = (LookupSource(context, source) ? AL_TRUE : AL_FALSE);

    ALCcontext_DecRef(context);

    return ret;
}


AL_API ALvoid AL_APIENTRY alSourcef(ALuint source, ALenum param, ALfloat value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(FloatValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
        SetSourcefv(Source, Context, param, &value);

    ALCcontext_DecRef(Context);
}

AL_API ALvoid AL_APIENTRY alSource3f(ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(FloatValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALfloat fvals[3] = { value1, value2, value3 };
        SetSourcefv(Source, Context, param, fvals);
    }

    ALCcontext_DecRef(Context);
}

AL_API ALvoid AL_APIENTRY alSourcefv(ALuint source, ALenum param, const ALfloat *values)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(FloatValsByProp(param) > 0))
        alSetError(Context, AL_INVALID_ENUM);
    else
        SetSourcefv(Source, Context, param, values);

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alSourcedSOFT(ALuint source, ALenum param, ALdouble value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(DoubleValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALfloat fval = (ALfloat)value;
        SetSourcefv(Source, Context, param, &fval);
    }

    ALCcontext_DecRef(Context);
}

AL_API ALvoid AL_APIENTRY alSource3dSOFT(ALuint source, ALenum param, ALdouble value1, ALdouble value2, ALdouble value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(DoubleValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALfloat fvals[3] = { (ALfloat)value1, (ALfloat)value2, (ALfloat)value3 };
        SetSourcefv(Source, Context, param, fvals);
    }

    ALCcontext_DecRef(Context);
}

AL_API ALvoid AL_APIENTRY alSourcedvSOFT(ALuint source, ALenum param, const ALdouble *values)
{
    ALCcontext *Context;
    ALsource   *Source;
    ALint      count;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!((count=DoubleValsByProp(param)) > 0 && count <= 6))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALfloat fvals[6];
        ALint i;

        for(i = 0;i < count;i++)
            fvals[i] = (ALfloat)values[i];
        SetSourcefv(Source, Context, param, fvals);
    }

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alSourcei(ALuint source, ALenum param, ALint value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(IntValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
        SetSourceiv(Source, Context, param, &value);

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alSource3i(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(IntValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALint ivals[3] = { value1, value2, value3 };
        SetSourceiv(Source, Context, param, ivals);
    }

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alSourceiv(ALuint source, ALenum param, const ALint *values)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(IntValsByProp(param) > 0))
        alSetError(Context, AL_INVALID_ENUM);
    else
        SetSourceiv(Source, Context, param, values);

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alSourcei64SOFT(ALuint source, ALenum param, ALint64SOFT value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(Int64ValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
        SetSourcei64v(Source, Context, param, &value);

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alSource3i64SOFT(ALuint source, ALenum param, ALint64SOFT value1, ALint64SOFT value2, ALint64SOFT value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(Int64ValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALint64SOFT i64vals[3] = { value1, value2, value3 };
        SetSourcei64v(Source, Context, param, i64vals);
    }

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alSourcei64vSOFT(ALuint source, ALenum param, const ALint64SOFT *values)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(Int64ValsByProp(param) > 0))
        alSetError(Context, AL_INVALID_ENUM);
    else
        SetSourcei64v(Source, Context, param, values);

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alGetSourcef(ALuint source, ALenum param, ALfloat *value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!value)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(FloatValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALdouble dval;
        if(GetSourcedv(Source, Context, param, &dval))
            *value = (ALfloat)dval;
    }

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alGetSource3f(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(value1 && value2 && value3))
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(FloatValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALdouble dvals[3];
        if(GetSourcedv(Source, Context, param, dvals))
        {
            *value1 = (ALfloat)dvals[0];
            *value2 = (ALfloat)dvals[1];
            *value3 = (ALfloat)dvals[2];
        }
    }

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alGetSourcefv(ALuint source, ALenum param, ALfloat *values)
{
    ALCcontext *Context;
    ALsource   *Source;
    ALint      count;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!((count=FloatValsByProp(param)) > 0 && count <= 6))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALdouble dvals[6];
        if(GetSourcedv(Source, Context, param, dvals))
        {
            ALint i;
            for(i = 0;i < count;i++)
                values[i] = (ALfloat)dvals[i];
        }
    }

    ALCcontext_DecRef(Context);
}


AL_API void AL_APIENTRY alGetSourcedSOFT(ALuint source, ALenum param, ALdouble *value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!value)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(DoubleValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
        GetSourcedv(Source, Context, param, value);

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alGetSource3dSOFT(ALuint source, ALenum param, ALdouble *value1, ALdouble *value2, ALdouble *value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(value1 && value2 && value3))
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(DoubleValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALdouble dvals[3];
        if(GetSourcedv(Source, Context, param, dvals))
        {
            *value1 = dvals[0];
            *value2 = dvals[1];
            *value3 = dvals[2];
        }
    }

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alGetSourcedvSOFT(ALuint source, ALenum param, ALdouble *values)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(DoubleValsByProp(param) > 0))
        alSetError(Context, AL_INVALID_ENUM);
    else
        GetSourcedv(Source, Context, param, values);

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alGetSourcei(ALuint source, ALenum param, ALint *value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!value)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(IntValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
        GetSourceiv(Source, Context, param, value);

    ALCcontext_DecRef(Context);
}


AL_API void AL_APIENTRY alGetSource3i(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(value1 && value2 && value3))
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(IntValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALint ivals[3];
        if(GetSourceiv(Source, Context, param, ivals))
        {
            *value1 = ivals[0];
            *value2 = ivals[1];
            *value3 = ivals[2];
        }
    }

    ALCcontext_DecRef(Context);
}


AL_API void AL_APIENTRY alGetSourceiv(ALuint source, ALenum param, ALint *values)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(IntValsByProp(param) > 0))
        alSetError(Context, AL_INVALID_ENUM);
    else
        GetSourceiv(Source, Context, param, values);

    ALCcontext_DecRef(Context);
}


AL_API void AL_APIENTRY alGetSourcei64SOFT(ALuint source, ALenum param, ALint64SOFT *value)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!value)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(Int64ValsByProp(param) == 1))
        alSetError(Context, AL_INVALID_ENUM);
    else
        GetSourcei64v(Source, Context, param, value);

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alGetSource3i64SOFT(ALuint source, ALenum param, ALint64SOFT *value1, ALint64SOFT *value2, ALint64SOFT *value3)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!(value1 && value2 && value3))
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(Int64ValsByProp(param) == 3))
        alSetError(Context, AL_INVALID_ENUM);
    else
    {
        ALint64 i64vals[3];
        if(GetSourcei64v(Source, Context, param, i64vals))
        {
            *value1 = i64vals[0];
            *value2 = i64vals[1];
            *value3 = i64vals[2];
        }
    }

    ALCcontext_DecRef(Context);
}

AL_API void AL_APIENTRY alGetSourcei64vSOFT(ALuint source, ALenum param, ALint64SOFT *values)
{
    ALCcontext *Context;
    ALsource   *Source;

    Context = GetContextRef();
    if(!Context) return;

    if((Source=LookupSource(Context, source)) == NULL)
        alSetError(Context, AL_INVALID_NAME);
    else if(!values)
        alSetError(Context, AL_INVALID_VALUE);
    else if(!(Int64ValsByProp(param) > 0))
        alSetError(Context, AL_INVALID_ENUM);
    else
        GetSourcei64v(Source, Context, param, values);

    ALCcontext_DecRef(Context);
}


AL_API ALvoid AL_APIENTRY alSourcePlay(ALuint source)
{
    alSourcePlayv(1, &source);
}
AL_API ALvoid AL_APIENTRY alSourcePlayv(ALsizei n, const ALuint *sources)
{
    ALCcontext *context;
    ALsource *source;
    ALsizei i;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
    for(i = 0;i < n;i++)
    {
        if(!LookupSource(context, sources[i]))
            SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    }

    LockContext(context);
    while(n > context->MaxVoices-context->VoiceCount)
    {
        ALvoice *temp = NULL;
        ALsizei newcount;

        newcount = context->MaxVoices << 1;
        if(newcount > 0)
            temp = realloc(context->Voices, newcount * sizeof(context->Voices[0]));
        if(!temp)
        {
            UnlockContext(context);
            SET_ERROR_AND_GOTO(context, AL_OUT_OF_MEMORY, done);
        }
        memset(&temp[context->MaxVoices], 0, (newcount-context->MaxVoices) * sizeof(temp[0]));

        context->Voices = temp;
        context->MaxVoices = newcount;
    }

    for(i = 0;i < n;i++)
    {
        source = LookupSource(context, sources[i]);
        if(context->DeferUpdates) source->new_state = AL_PLAYING;
        else SetSourceState(source, context, AL_PLAYING);
    }
    UnlockContext(context);

done:
    ALCcontext_DecRef(context);
}

AL_API ALvoid AL_APIENTRY alSourcePause(ALuint source)
{
    alSourcePausev(1, &source);
}
AL_API ALvoid AL_APIENTRY alSourcePausev(ALsizei n, const ALuint *sources)
{
    ALCcontext *context;
    ALsource *source;
    ALsizei i;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
    for(i = 0;i < n;i++)
    {
        if(!LookupSource(context, sources[i]))
            SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    }

    LockContext(context);
    for(i = 0;i < n;i++)
    {
        source = LookupSource(context, sources[i]);
        if(context->DeferUpdates) source->new_state = AL_PAUSED;
        else SetSourceState(source, context, AL_PAUSED);
    }
    UnlockContext(context);

done:
    ALCcontext_DecRef(context);
}

AL_API ALvoid AL_APIENTRY alSourceStop(ALuint source)
{
    alSourceStopv(1, &source);
}
AL_API ALvoid AL_APIENTRY alSourceStopv(ALsizei n, const ALuint *sources)
{
    ALCcontext *context;
    ALsource *source;
    ALsizei i;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
    for(i = 0;i < n;i++)
    {
        if(!LookupSource(context, sources[i]))
            SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    }

    LockContext(context);
    for(i = 0;i < n;i++)
    {
        source = LookupSource(context, sources[i]);
        source->new_state = AL_NONE;
        SetSourceState(source, context, AL_STOPPED);
    }
    UnlockContext(context);

done:
    ALCcontext_DecRef(context);
}

AL_API ALvoid AL_APIENTRY alSourceRewind(ALuint source)
{
    alSourceRewindv(1, &source);
}
AL_API ALvoid AL_APIENTRY alSourceRewindv(ALsizei n, const ALuint *sources)
{
    ALCcontext *context;
    ALsource *source;
    ALsizei i;

    context = GetContextRef();
    if(!context) return;

    if(!(n >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
    for(i = 0;i < n;i++)
    {
        if(!LookupSource(context, sources[i]))
            SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);
    }

    LockContext(context);
    for(i = 0;i < n;i++)
    {
        source = LookupSource(context, sources[i]);
        source->new_state = AL_NONE;
        SetSourceState(source, context, AL_INITIAL);
    }
    UnlockContext(context);

done:
    ALCcontext_DecRef(context);
}


AL_API ALvoid AL_APIENTRY alSourceQueueBuffers(ALuint src, ALsizei nb, const ALuint *buffers)
{
    ALCdevice *device;
    ALCcontext *context;
    ALsource *source;
    ALsizei i;
    ALbufferlistitem *BufferListStart;
    ALbufferlistitem *BufferList;
    ALbuffer *BufferFmt = NULL;

    if(nb == 0)
        return;

    context = GetContextRef();
    if(!context) return;

    device = context->Device;

    if(!(nb >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
    if((source=LookupSource(context, src)) == NULL)
        SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);

    WriteLock(&source->queue_lock);
    if(source->SourceType == AL_STATIC)
    {
        WriteUnlock(&source->queue_lock);
        /* Can't queue on a Static Source */
        SET_ERROR_AND_GOTO(context, AL_INVALID_OPERATION, done);
    }

    /* Check for a valid Buffer, for its frequency and format */
    BufferList = ATOMIC_LOAD(&source->queue);
    while(BufferList)
    {
        if(BufferList->buffer)
        {
            BufferFmt = BufferList->buffer;
            break;
        }
        BufferList = BufferList->next;
    }

    BufferListStart = NULL;
    BufferList = NULL;
    for(i = 0;i < nb;i++)
    {
        ALbuffer *buffer = NULL;
        if(buffers[i] && (buffer=LookupBuffer(device, buffers[i])) == NULL)
        {
            WriteUnlock(&source->queue_lock);
            SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, buffer_error);
        }

        if(!BufferListStart)
        {
            BufferListStart = malloc(sizeof(ALbufferlistitem));
            BufferListStart->buffer = buffer;
            BufferListStart->next = NULL;
            BufferListStart->prev = NULL;
            BufferList = BufferListStart;
        }
        else
        {
            BufferList->next = malloc(sizeof(ALbufferlistitem));
            BufferList->next->buffer = buffer;
            BufferList->next->next = NULL;
            BufferList->next->prev = BufferList;
            BufferList = BufferList->next;
        }
        if(!buffer) continue;

        /* Hold a read lock on each buffer being queued while checking all
         * provided buffers. This is done so other threads don't see an extra
         * reference on some buffers if this operation ends up failing. */
        ReadLock(&buffer->lock);
        IncrementRef(&buffer->ref);

        if(BufferFmt == NULL)
        {
            BufferFmt = buffer;

            source->NumChannels = ChannelsFromFmt(buffer->FmtChannels);
            source->SampleSize  = BytesFromFmt(buffer->FmtType);
        }
        else if(BufferFmt->Frequency != buffer->Frequency ||
                BufferFmt->OriginalChannels != buffer->OriginalChannels ||
                BufferFmt->OriginalType != buffer->OriginalType)
        {
            WriteUnlock(&source->queue_lock);
            SET_ERROR_AND_GOTO(context, AL_INVALID_OPERATION, buffer_error);

        buffer_error:
            /* A buffer failed (invalid ID or format), so unlock and release
             * each buffer we had. */
            while(BufferList != NULL)
            {
                ALbufferlistitem *prev = BufferList->prev;
                if((buffer=BufferList->buffer) != NULL)
                {
                    DecrementRef(&buffer->ref);
                    ReadUnlock(&buffer->lock);
                }
                free(BufferList);
                BufferList = prev;
            }
            goto done;
        }
    }
    /* All buffers good, unlock them now. */
    while(BufferList != NULL)
    {
        ALbuffer *buffer = BufferList->buffer;
        if(buffer) ReadUnlock(&buffer->lock);
        BufferList = BufferList->prev;
    }

    /* Source is now streaming */
    source->SourceType = AL_STREAMING;

    BufferList = NULL;
    if(!ATOMIC_COMPARE_EXCHANGE_STRONG(ALbufferlistitem*, &source->queue, &BufferList, BufferListStart))
    {
        /* Queue head is not NULL, append to the end of the queue */
        while(BufferList->next != NULL)
            BufferList = BufferList->next;

        BufferListStart->prev = BufferList;
        BufferList->next = BufferListStart;
    }
    BufferList = NULL;
    ATOMIC_COMPARE_EXCHANGE_STRONG(ALbufferlistitem*, &source->current_buffer, &BufferList, BufferListStart);
    WriteUnlock(&source->queue_lock);

done:
    ALCcontext_DecRef(context);
}

AL_API ALvoid AL_APIENTRY alSourceUnqueueBuffers(ALuint src, ALsizei nb, ALuint *buffers)
{
    ALCcontext *context;
    ALsource *source;
    ALbufferlistitem *NewHead;
    ALbufferlistitem *OldHead;
    ALbufferlistitem *Current;
    ALsizei i;

    if(nb == 0)
        return;

    context = GetContextRef();
    if(!context) return;

    if(!(nb >= 0))
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);

    if((source=LookupSource(context, src)) == NULL)
        SET_ERROR_AND_GOTO(context, AL_INVALID_NAME, done);

    WriteLock(&source->queue_lock);
    /* Find the new buffer queue head */
    NewHead = ATOMIC_LOAD(&source->queue);
    Current = ATOMIC_LOAD(&source->current_buffer);
    for(i = 0;i < nb && NewHead;i++)
    {
        if(NewHead == Current)
            break;
        NewHead = NewHead->next;
    }
    if(source->Looping || source->SourceType != AL_STREAMING || i != nb)
    {
        WriteUnlock(&source->queue_lock);
        /* Trying to unqueue pending buffers, or a buffer that wasn't queued. */
        SET_ERROR_AND_GOTO(context, AL_INVALID_VALUE, done);
    }

    /* Swap it, and cut the new head from the old. */
    OldHead = ATOMIC_EXCHANGE(ALbufferlistitem*, &source->queue, NewHead);
    if(NewHead)
    {
        ALCdevice *device = context->Device;
        ALbufferlistitem *OldTail = NewHead->prev;
        uint count;

        /* Cut the new head's link back to the old body. The mixer is robust
         * enough to handle the link back going away. Once the active mix (if
         * any) is complete, it's safe to finish cutting the old tail from the
         * new head. */
        NewHead->prev = NULL;
        if(((count=ReadRef(&device->MixCount))&1) != 0)
        {
            while(count == ReadRef(&device->MixCount))
                althrd_yield();
        }
        OldTail->next = NULL;
    }
    WriteUnlock(&source->queue_lock);

    while(OldHead != NULL)
    {
        ALbufferlistitem *next = OldHead->next;
        ALbuffer *buffer = OldHead->buffer;

        if(!buffer)
            *(buffers++) = 0;
        else
        {
            *(buffers++) = buffer->id;
            DecrementRef(&buffer->ref);
        }

        free(OldHead);
        OldHead = next;
    }

done:
    ALCcontext_DecRef(context);
}


static ALvoid InitSourceParams(ALsource *Source)
{
    ALuint i;

    RWLockInit(&Source->queue_lock);

    Source->InnerAngle = 360.0f;
    Source->OuterAngle = 360.0f;
    Source->Pitch = 1.0f;
    aluVectorSet(&Source->Position, 0.0f, 0.0f, 0.0f, 1.0f);
    aluVectorSet(&Source->Velocity, 0.0f, 0.0f, 0.0f, 0.0f);
    aluVectorSet(&Source->Direction, 0.0f, 0.0f, 0.0f, 0.0f);
    Source->Orientation[0][0] =  0.0f;
    Source->Orientation[0][1] =  0.0f;
    Source->Orientation[0][2] = -1.0f;
    Source->Orientation[1][0] =  0.0f;
    Source->Orientation[1][1] =  1.0f;
    Source->Orientation[1][2] =  0.0f;
    Source->RefDistance = 1.0f;
    Source->MaxDistance = FLT_MAX;
    Source->RollOffFactor = 1.0f;
    Source->Looping = AL_FALSE;
    Source->Gain = 1.0f;
    Source->MinGain = 0.0f;
    Source->MaxGain = 1.0f;
    Source->OuterGain = 0.0f;
    Source->OuterGainHF = 1.0f;
    Source->Balance = 0.0f;

    Source->DryGainHFAuto = AL_TRUE;
    Source->WetGainAuto = AL_TRUE;
    Source->WetGainHFAuto = AL_TRUE;
    Source->AirAbsorptionFactor = 0.0f;
    Source->RoomRolloffFactor = 0.0f;
    Source->DopplerFactor = 1.0f;
    Source->DirectChannels = AL_FALSE;

    Source->Radius = 0.0f;

    Source->DistanceModel = DefaultDistanceModel;

    Source->state = AL_INITIAL;
    Source->new_state = AL_NONE;
    Source->SourceType = AL_UNDETERMINED;
    Source->Offset = -1.0;

    ATOMIC_INIT(&Source->queue, NULL);
    ATOMIC_INIT(&Source->current_buffer, NULL);

    Source->Direct.Gain = 1.0f;
    Source->Direct.GainHF = 1.0f;
    Source->Direct.HFReference = LOWPASSFREQREF;
    Source->Direct.GainLF = 1.0f;
    Source->Direct.LFReference = HIGHPASSFREQREF;
    for(i = 0;i < MAX_SENDS;i++)
    {
        Source->Send[i].Gain = 1.0f;
        Source->Send[i].GainHF = 1.0f;
        Source->Send[i].HFReference = LOWPASSFREQREF;
        Source->Send[i].GainLF = 1.0f;
        Source->Send[i].LFReference = HIGHPASSFREQREF;
    }

    ATOMIC_INIT(&Source->NeedsUpdate, AL_TRUE);
}


/* SetSourceState
 *
 * Sets the source's new play state given its current state.
 */
ALvoid SetSourceState(ALsource *Source, ALCcontext *Context, ALenum state)
{
    WriteLock(&Source->queue_lock);
    if(state == AL_PLAYING)
    {
        ALCdevice *device = Context->Device;
        ALbufferlistitem *BufferList;
        ALboolean discontinuity;
        ALvoice *voice = NULL;
        ALsizei i;

        /* Check that there is a queue containing at least one valid, non zero
         * length Buffer. */
        BufferList = ATOMIC_LOAD(&Source->queue);
        while(BufferList)
        {
            ALbuffer *buffer;
            if((buffer=BufferList->buffer) != NULL && buffer->SampleLen > 0)
                break;
            BufferList = BufferList->next;
        }

        if(Source->state != AL_PAUSED)
        {
            Source->state = AL_PLAYING;
            Source->position = 0;
            Source->position_fraction = 0;
            ATOMIC_STORE(&Source->current_buffer, BufferList);
            discontinuity = AL_TRUE;
        }
        else
        {
            Source->state = AL_PLAYING;
            discontinuity = AL_FALSE;
        }

        // Check if an Offset has been set
        if(Source->Offset >= 0.0)
        {
            ApplyOffset(Source);
            /* discontinuity = AL_TRUE;??? */
        }

        /* If there's nothing to play, or device is disconnected, go right to
         * stopped */
        if(!BufferList || !device->Connected)
            goto do_stop;

        /* Make sure this source isn't already active, while looking for an
         * unused active source slot to put it in. */
        for(i = 0;i < Context->VoiceCount;i++)
        {
            ALsource *old = Source;
            if(COMPARE_EXCHANGE(&Context->Voices[i].Source, &old, NULL))
            {
                if(voice == NULL)
                {
                    voice = &Context->Voices[i];
                    voice->Source = Source;
                }
                break;
            }
            old = NULL;
            if(voice == NULL && COMPARE_EXCHANGE(&Context->Voices[i].Source, &old, Source))
                voice = &Context->Voices[i];
        }
        if(voice == NULL)
        {
            voice = &Context->Voices[Context->VoiceCount++];
            voice->Source = Source;
        }

        /* Clear previous samples if playback is discontinuous. */
        if(discontinuity)
            memset(voice->PrevSamples, 0, sizeof(voice->PrevSamples));

        voice->Direct.Moving  = AL_FALSE;
        voice->Direct.Counter = 0;
        for(i = 0;i < MAX_INPUT_CHANNELS;i++)
        {
            ALsizei j;
            for(j = 0;j < HRTF_HISTORY_LENGTH;j++)
                voice->Direct.Hrtf[i].State.History[j] = 0.0f;
            for(j = 0;j < HRIR_LENGTH;j++)
            {
                voice->Direct.Hrtf[i].State.Values[j][0] = 0.0f;
                voice->Direct.Hrtf[i].State.Values[j][1] = 0.0f;
            }
        }
        for(i = 0;i < (ALsizei)device->NumAuxSends;i++)
        {
            voice->Send[i].Moving  = AL_FALSE;
            voice->Send[i].Counter = 0;
        }

        if(BufferList->buffer->FmtChannels == FmtMono)
            voice->Update = CalcSourceParams;
        else
            voice->Update = CalcNonAttnSourceParams;

        ATOMIC_STORE(&Source->NeedsUpdate, AL_TRUE);
    }
    else if(state == AL_PAUSED)
    {
        if(Source->state == AL_PLAYING)
            Source->state = AL_PAUSED;
    }
    else if(state == AL_STOPPED)
    {
    do_stop:
        if(Source->state != AL_INITIAL)
        {
            Source->state = AL_STOPPED;
            ATOMIC_STORE(&Source->current_buffer, NULL);
        }
        Source->Offset = -1.0;
    }
    else if(state == AL_INITIAL)
    {
        if(Source->state != AL_INITIAL)
        {
            Source->state = AL_INITIAL;
            Source->position = 0;
            Source->position_fraction = 0;
            ATOMIC_STORE(&Source->current_buffer, ATOMIC_LOAD(&Source->queue));
        }
        Source->Offset = -1.0;
    }
    WriteUnlock(&Source->queue_lock);
}

/* GetSourceSampleOffset
 *
 * Gets the current read offset for the given Source, in 32.32 fixed-point
 * samples. The offset is relative to the start of the queue (not the start of
 * the current buffer).
 */
ALint64 GetSourceSampleOffset(ALsource *Source)
{
    const ALbufferlistitem *BufferList;
    const ALbufferlistitem *Current;
    ALuint64 readPos;

    ReadLock(&Source->queue_lock);
    if(Source->state != AL_PLAYING && Source->state != AL_PAUSED)
    {
        ReadUnlock(&Source->queue_lock);
        return 0;
    }

    /* NOTE: This is the offset into the *current* buffer, so add the length of
     * any played buffers */
    readPos  = (ALuint64)Source->position << 32;
    readPos |= (ALuint64)Source->position_fraction << (32-FRACTIONBITS);
    BufferList = ATOMIC_LOAD(&Source->queue);
    Current = ATOMIC_LOAD(&Source->current_buffer);
    while(BufferList && BufferList != Current)
    {
        if(BufferList->buffer)
            readPos += (ALuint64)BufferList->buffer->SampleLen << 32;
        BufferList = BufferList->next;
    }

    ReadUnlock(&Source->queue_lock);
    return (ALint64)minu64(readPos, U64(0x7fffffffffffffff));
}

/* GetSourceSecOffset
 *
 * Gets the current read offset for the given Source, in seconds. The offset is
 * relative to the start of the queue (not the start of the current buffer).
 */
static ALdouble GetSourceSecOffset(ALsource *Source)
{
    const ALbufferlistitem *BufferList;
    const ALbufferlistitem *Current;
    const ALbuffer *Buffer = NULL;
    ALuint64 readPos;

    ReadLock(&Source->queue_lock);
    if(Source->state != AL_PLAYING && Source->state != AL_PAUSED)
    {
        ReadUnlock(&Source->queue_lock);
        return 0.0;
    }

    /* NOTE: This is the offset into the *current* buffer, so add the length of
     * any played buffers */
    readPos  = (ALuint64)Source->position << FRACTIONBITS;
    readPos |= (ALuint64)Source->position_fraction;
    BufferList = ATOMIC_LOAD(&Source->queue);
    Current = ATOMIC_LOAD(&Source->current_buffer);
    while(BufferList && BufferList != Current)
    {
        const ALbuffer *buffer = BufferList->buffer;
        if(buffer != NULL)
        {
            if(!Buffer) Buffer = buffer;
            readPos += (ALuint64)buffer->SampleLen << FRACTIONBITS;
        }
        BufferList = BufferList->next;
    }

    while(BufferList && !Buffer)
    {
        Buffer = BufferList->buffer;
        BufferList = BufferList->next;
    }
    assert(Buffer != NULL);

    ReadUnlock(&Source->queue_lock);
    return (ALdouble)readPos / (ALdouble)FRACTIONONE / (ALdouble)Buffer->Frequency;
}

/* GetSourceOffsets
 *
 * Gets the current read and write offsets for the given Source, in the
 * appropriate format (Bytes, Samples or Seconds). The offsets are relative to
 * the start of the queue (not the start of the current buffer).
 */
static ALvoid GetSourceOffsets(ALsource *Source, ALenum name, ALdouble *offset, ALdouble updateLen)
{
    const ALbufferlistitem *BufferList;
    const ALbufferlistitem *Current;
    const ALbuffer *Buffer = NULL;
    ALboolean readFin = AL_FALSE;
    ALuint readPos, readPosFrac, writePos;
    ALuint totalBufferLen;

    ReadLock(&Source->queue_lock);
    if(Source->state != AL_PLAYING && Source->state != AL_PAUSED)
    {
        offset[0] = 0.0;
        offset[1] = 0.0;
        ReadUnlock(&Source->queue_lock);
        return;
    }

    if(updateLen > 0.0 && updateLen < 0.015)
        updateLen = 0.015;

    /* NOTE: This is the offset into the *current* buffer, so add the length of
     * any played buffers */
    totalBufferLen = 0;
    readPos = Source->position;
    readPosFrac = Source->position_fraction;
    BufferList = ATOMIC_LOAD(&Source->queue);
    Current = ATOMIC_LOAD(&Source->current_buffer);
    while(BufferList != NULL)
    {
        const ALbuffer *buffer;
        readFin = readFin || (BufferList == Current);
        if((buffer=BufferList->buffer) != NULL)
        {
            if(!Buffer) Buffer = buffer;
            totalBufferLen += buffer->SampleLen;
            if(!readFin) readPos += buffer->SampleLen;
        }
        BufferList = BufferList->next;
    }
    assert(Buffer != NULL);

    if(Source->state == AL_PLAYING)
        writePos = readPos + (ALuint)(updateLen*Buffer->Frequency + 0.5f);
    else
        writePos = readPos;

    if(Source->Looping)
    {
        readPos %= totalBufferLen;
        writePos %= totalBufferLen;
    }
    else
    {
        /* Wrap positions back to 0 */
        if(readPos >= totalBufferLen)
            readPos = readPosFrac = 0;
        if(writePos >= totalBufferLen)
            writePos = 0;
    }

    switch(name)
    {
        case AL_SEC_OFFSET:
            offset[0] = (readPos + (ALdouble)readPosFrac/FRACTIONONE)/Buffer->Frequency;
            offset[1] = (ALdouble)writePos/Buffer->Frequency;
            break;

        case AL_SAMPLE_OFFSET:
        case AL_SAMPLE_RW_OFFSETS_SOFT:
            offset[0] = readPos + (ALdouble)readPosFrac/FRACTIONONE;
            offset[1] = (ALdouble)writePos;
            break;

        case AL_BYTE_OFFSET:
        case AL_BYTE_RW_OFFSETS_SOFT:
            if(Buffer->OriginalType == UserFmtIMA4)
            {
                ALsizei align = (Buffer->OriginalAlign-1)/2 + 4;
                ALuint BlockSize = align * ChannelsFromFmt(Buffer->FmtChannels);
                ALuint FrameBlockSize = Buffer->OriginalAlign;

                /* Round down to nearest ADPCM block */
                offset[0] = (ALdouble)(readPos / FrameBlockSize * BlockSize);
                if(Source->state != AL_PLAYING)
                    offset[1] = offset[0];
                else
                {
                    /* Round up to nearest ADPCM block */
                    offset[1] = (ALdouble)((writePos+FrameBlockSize-1) /
                                           FrameBlockSize * BlockSize);
                }
            }
            else if(Buffer->OriginalType == UserFmtMSADPCM)
            {
                ALsizei align = (Buffer->OriginalAlign-2)/2 + 7;
                ALuint BlockSize = align * ChannelsFromFmt(Buffer->FmtChannels);
                ALuint FrameBlockSize = Buffer->OriginalAlign;

                /* Round down to nearest ADPCM block */
                offset[0] = (ALdouble)(readPos / FrameBlockSize * BlockSize);
                if(Source->state != AL_PLAYING)
                    offset[1] = offset[0];
                else
                {
                    /* Round up to nearest ADPCM block */
                    offset[1] = (ALdouble)((writePos+FrameBlockSize-1) /
                                           FrameBlockSize * BlockSize);
                }
            }
            else
            {
                ALuint FrameSize = FrameSizeFromUserFmt(Buffer->OriginalChannels, Buffer->OriginalType);
                offset[0] = (ALdouble)(readPos * FrameSize);
                offset[1] = (ALdouble)(writePos * FrameSize);
            }
            break;
    }

    ReadUnlock(&Source->queue_lock);
}


/* ApplyOffset
 *
 * Apply the stored playback offset to the Source. This function will update
 * the number of buffers "played" given the stored offset.
 */
ALboolean ApplyOffset(ALsource *Source)
{
    ALbufferlistitem *BufferList;
    const ALbuffer *Buffer;
    ALuint bufferLen, totalBufferLen;
    ALuint offset=0, frac=0;

    /* Get sample frame offset */
    if(!GetSampleOffset(Source, &offset, &frac))
        return AL_FALSE;

    totalBufferLen = 0;
    BufferList = ATOMIC_LOAD(&Source->queue);
    while(BufferList && totalBufferLen <= offset)
    {
        Buffer = BufferList->buffer;
        bufferLen = Buffer ? Buffer->SampleLen : 0;

        if(bufferLen > offset-totalBufferLen)
        {
            /* Offset is in this buffer */
            ATOMIC_STORE(&Source->current_buffer, BufferList);

            Source->position = offset - totalBufferLen;
            Source->position_fraction = frac;
            return AL_TRUE;
        }

        totalBufferLen += bufferLen;

        BufferList = BufferList->next;
    }

    /* Offset is out of range of the queue */
    return AL_FALSE;
}


/* GetSampleOffset
 *
 * Retrieves the sample offset into the Source's queue (from the Sample, Byte
 * or Second offset supplied by the application). This takes into account the
 * fact that the buffer format may have been modifed since.
 */
static ALboolean GetSampleOffset(ALsource *Source, ALuint *offset, ALuint *frac)
{
    const ALbuffer *Buffer = NULL;
    const ALbufferlistitem *BufferList;
    ALdouble dbloff, dblfrac;

    /* Find the first valid Buffer in the Queue */
    BufferList = ATOMIC_LOAD(&Source->queue);
    while(BufferList)
    {
        if(BufferList->buffer)
        {
            Buffer = BufferList->buffer;
            break;
        }
        BufferList = BufferList->next;
    }
    if(!Buffer)
    {
        Source->Offset = -1.0;
        return AL_FALSE;
    }

    switch(Source->OffsetType)
    {
    case AL_BYTE_OFFSET:
        /* Determine the ByteOffset (and ensure it is block aligned) */
        *offset = (ALuint)Source->Offset;
        if(Buffer->OriginalType == UserFmtIMA4)
        {
            ALsizei align = (Buffer->OriginalAlign-1)/2 + 4;
            *offset /= align * ChannelsFromUserFmt(Buffer->OriginalChannels);
            *offset *= Buffer->OriginalAlign;
        }
        else if(Buffer->OriginalType == UserFmtMSADPCM)
        {
            ALsizei align = (Buffer->OriginalAlign-2)/2 + 7;
            *offset /= align * ChannelsFromUserFmt(Buffer->OriginalChannels);
            *offset *= Buffer->OriginalAlign;
        }
        else
            *offset /= FrameSizeFromUserFmt(Buffer->OriginalChannels, Buffer->OriginalType);
        *frac = 0;
        break;

    case AL_SAMPLE_OFFSET:
        dblfrac = modf(Source->Offset, &dbloff);
        *offset = (ALuint)mind(dbloff, UINT_MAX);
        *frac = (ALuint)mind(dblfrac*FRACTIONONE, FRACTIONONE-1.0);
        break;

    case AL_SEC_OFFSET:
        dblfrac = modf(Source->Offset*Buffer->Frequency, &dbloff);
        *offset = (ALuint)mind(dbloff, UINT_MAX);
        *frac = (ALuint)mind(dblfrac*FRACTIONONE, FRACTIONONE-1.0);
        break;
    }
    Source->Offset = -1.0;

    return AL_TRUE;
}


/* ReleaseALSources
 *
 * Destroys all sources in the source map.
 */
ALvoid ReleaseALSources(ALCcontext *Context)
{
    ALbufferlistitem *item;
    ALsizei pos;
    ALuint j;
    for(pos = 0;pos < Context->SourceMap.size;pos++)
    {
        ALsource *temp = Context->SourceMap.array[pos].value;
        Context->SourceMap.array[pos].value = NULL;

        item = ATOMIC_EXCHANGE(ALbufferlistitem*, &temp->queue, NULL);
        while(item != NULL)
        {
            ALbufferlistitem *next = item->next;
            if(item->buffer != NULL)
                DecrementRef(&item->buffer->ref);
            free(item);
            item = next;
        }

        for(j = 0;j < MAX_SENDS;++j)
        {
            if(temp->Send[j].Slot)
                DecrementRef(&temp->Send[j].Slot->ref);
            temp->Send[j].Slot = NULL;
        }

        FreeThunkEntry(temp->id);
        memset(temp, 0, sizeof(*temp));
        al_free(temp);
    }
}
