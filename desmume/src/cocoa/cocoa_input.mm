/*
	Copyright (C) 2011 Roger Manuel
	Copyright (C) 2012-2016 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#import "cocoa_input.h"
#import "cocoa_globals.h"

#include "mic_ext.h"
#include "coreaudiosound.h"
#include "audiosamplegenerator.h"
#include "../NDSSystem.h"
#include "../slot2.h"
#undef BOOL

NullGenerator nullSampleGenerator;
InternalNoiseGenerator internalNoiseGenerator;
WhiteNoiseGenerator whiteNoiseGenerator;
SineWaveGenerator sineWaveGenerator(250.0, MIC_SAMPLE_RATE);


@implementation CocoaDSController

@synthesize delegate;
@dynamic autohold;
@synthesize paddleAdjust;
@synthesize stylusPressure;
@synthesize stylusEnableJitter;
@dynamic isHardwareMicAvailable;
@dynamic isHardwareMicIdle;
@dynamic isHardwareMicInClip;
@synthesize micLevel;
@dynamic hardwareMicEnabled;
@dynamic hardwareMicLocked;
@dynamic hardwareMicGain;
@synthesize hardwareMicMute;
@dynamic hardwareMicPause;
@dynamic softwareMicState;
@dynamic softwareMicMode;
@synthesize micMode;
@synthesize CAInputDevice;
@synthesize softwareMicSampleGenerator;
@synthesize selectedAudioFileGenerator;
@synthesize hardwareMicInfoString;
@synthesize hardwareMicNameString;
@synthesize hardwareMicManufacturerString;
@synthesize hardwareMicSampleRateString;

- (id)init
{
	self = [super init];
	if (self == nil)
	{
		return self;
	}
	
	for (size_t i = 0; i < DSControllerState_StatesCount; i++)
	{
		ndsInput[i].state = false;
		ndsInput[i].turbo = false;
		ndsInput[i].turboPattern = false;
		ndsInput[i].autohold = false;
	}
	
	delegate = nil;
	spinlockControllerState = OS_SPINLOCK_INIT;	
	autohold = NO;
	_isAutoholdCleared = YES;
	_useHardwareMic = NO;
	_availableMicSamples = 0;
	
	_hwMicLevelList = new std::vector<uint8_t>;
	_hwMicLevelList->reserve(1024);
	_hwMicLevelList->clear();
	micLevel = 0.0f;
	
	micMode = MICMODE_NONE;
	selectedAudioFileGenerator = NULL;
	CAInputDevice = new CoreAudioInput;
	CAInputDevice->SetCallbackHardwareStateChanged(&CAHardwareStateChangedCallback, self, NULL);
	CAInputDevice->SetCallbackHardwareGainChanged(&CAHardwareGainChangedCallback, self, NULL);
	softwareMicSampleGenerator = &nullSampleGenerator;
	touchLocation = NSMakePoint(0.0f, 0.0f);
	paddleAdjust = 0;
	
	hardwareMicInfoString = @"No hardware input detected.";
	hardwareMicNameString = @"No hardware input detected.";
	hardwareMicManufacturerString = @"No hardware input detected.";
	hardwareMicSampleRateString = @"No hardware input detected.";
	
	Mic_SetResetCallback(&CAResetCallback, self, NULL);
	Mic_SetSampleReadCallback(&CASampleReadCallback, self, NULL);
	
	return self;
}

- (void)dealloc
{
	delete CAInputDevice;
	delete _hwMicLevelList;
	[super dealloc];
}

- (BOOL) isHardwareMicAvailable
{
	return ( CAInputDevice->IsHardwareEnabled() &&
			!CAInputDevice->IsHardwareLocked() &&
			!CAInputDevice->GetPauseState() ) ? YES : NO;
}

- (BOOL) isHardwareMicIdle
{
	return (micLevel < MIC_NULL_LEVEL_THRESHOLD);
}

- (BOOL) isHardwareMicInClip
{
	return (micLevel >= MIC_CLIP_LEVEL_THRESHOLD);
}

- (void) setHardwareMicEnabled:(BOOL)micEnabled
{
	if (micEnabled)
	{
		CAInputDevice->Start();
	}
	else
	{
		CAInputDevice->Stop();
	}
}

- (BOOL) hardwareMicEnabled
{
	return (CAInputDevice->IsHardwareEnabled()) ? YES : NO;
}

- (BOOL) hardwareMicLocked
{
	return (CAInputDevice->IsHardwareLocked()) ? YES : NO;
}

- (void) setHardwareMicGain:(float)micGain
{
	CAInputDevice->SetGain(micGain);
}

- (float) hardwareMicGain
{
	return CAInputDevice->GetGain();
}

- (void) setHardwareMicPause:(BOOL)isMicPaused
{
	bool pauseState = (isMicPaused || [self hardwareMicMute]) ? true : false;
	CAInputDevice->SetPauseState(pauseState);
}

- (BOOL) hardwareMicPause
{
	return (CAInputDevice->GetPauseState()) ? YES : NO;
}

- (void) setSoftwareMicState:(BOOL)theState
{
	OSSpinLockLock(&spinlockControllerState);
	ndsInput[DSControllerState_Microphone].state = (theState) ? true : false;
	OSSpinLockUnlock(&spinlockControllerState);
}

- (BOOL) softwareMicState
{
	OSSpinLockLock(&spinlockControllerState);
	BOOL theState = (ndsInput[DSControllerState_Microphone].state) ? YES : NO;
	OSSpinLockUnlock(&spinlockControllerState);
	return theState;
}

- (void) setSoftwareMicMode:(NSInteger)theMode
{
	OSSpinLockLock(&spinlockControllerState);
	micMode = theMode;
	OSSpinLockUnlock(&spinlockControllerState);
}

- (NSInteger) softwareMicMode
{
	OSSpinLockLock(&spinlockControllerState);
	NSInteger theMode = micMode;
	OSSpinLockUnlock(&spinlockControllerState);
	return theMode;
}

- (void) setAutohold:(BOOL)theState
{
	OSSpinLockLock(&spinlockControllerState);
	
	autohold = theState;
	
	if (autohold && _isAutoholdCleared)
	{
		memset(ndsInput, 0, sizeof(ndsInput));
		_isAutoholdCleared = NO;
	}
	
	if (!autohold)
	{
		for (size_t i = 0; i < DSControllerState_StatesCount; i++)
		{
			ndsInput[i].state = ndsInput[i].autohold;
		}
	}
	
	OSSpinLockUnlock(&spinlockControllerState);
}

- (BOOL) autohold
{
	OSSpinLockLock(&spinlockControllerState);
	const BOOL theState = autohold;
	OSSpinLockUnlock(&spinlockControllerState);
	return theState;
}

- (void) setControllerState:(BOOL)theState controlID:(const NSUInteger)controlID
{
	[self setControllerState:theState controlID:controlID turbo:NO];
}

- (void) setControllerState:(BOOL)theState controlID:(const NSUInteger)controlID turbo:(const BOOL)isTurboEnabled
{
	if (controlID >= DSControllerState_StatesCount)
	{
		return;
	}
	
	OSSpinLockLock(&spinlockControllerState);
	
	if (autohold)
	{
		if (theState)
		{
			ndsInput[controlID].turbo = (isTurboEnabled) ? true : false;
			ndsInput[controlID].turboPattern = (ndsInput[controlID].turbo) ? 0x5555 : 0;
			ndsInput[controlID].autohold = true;
		}
	}
	else
	{
		ndsInput[controlID].state = (theState || ndsInput[controlID].autohold);
		ndsInput[controlID].turbo = (isTurboEnabled && ndsInput[controlID].state);
		ndsInput[controlID].turboPattern = (ndsInput[controlID].turbo) ? 0x5555 : 0;
	}
	
	OSSpinLockUnlock(&spinlockControllerState);
}

- (void) setTouchState:(BOOL)theState location:(const NSPoint)theLocation
{
	OSSpinLockLock(&spinlockControllerState);
	ndsInput[DSControllerState_Touch].state = (theState) ? true : false;
	touchLocation = theLocation;
	OSSpinLockUnlock(&spinlockControllerState);
}

- (void) setSineWaveGeneratorFrequency:(const double)freq
{
	sineWaveGenerator.setFrequency(freq);
}

- (void) clearAutohold
{
	[self setAutohold:NO];
	OSSpinLockLock(&spinlockControllerState);
	
	if (!_isAutoholdCleared)
	{
		memset(ndsInput, 0, sizeof(ndsInput));
		_isAutoholdCleared = YES;
	}
	
	OSSpinLockUnlock(&spinlockControllerState);
}

- (void) flush
{
	OSSpinLockLock(&spinlockControllerState);
	
	const NSPoint theLocation = touchLocation;
	const NSInteger theMicMode = micMode;
	static bool flushedStates[DSControllerState_StatesCount] = {0};
	
	if (!autohold)
	{
		for (size_t i = 0; i < DSControllerState_StatesCount; i++)
		{
			flushedStates[i] = (ndsInput[i].state || ndsInput[i].autohold);
			
			if (ndsInput[i].turbo)
			{
				const bool turboState = ndsInput[i].turboPattern & 0x0001;
				flushedStates[i] = (flushedStates[i] && turboState);
				ndsInput[i].turboPattern >>= 1;
				ndsInput[i].turboPattern |= (turboState) ? 0x8000 : 0x0000;
			}
			else
			{
				flushedStates[i] = ndsInput[i].state;
			}
		}
	}
	
	OSSpinLockUnlock(&spinlockControllerState);
	
	const bool isTouchDown = flushedStates[DSControllerState_Touch];
	const bool isMicPressed = flushedStates[DSControllerState_Microphone];
	
	// Setup the DS pad.
	NDS_setPad(flushedStates[DSControllerState_Right],
			   flushedStates[DSControllerState_Left],
			   flushedStates[DSControllerState_Down],
			   flushedStates[DSControllerState_Up],
			   flushedStates[DSControllerState_Select],
			   flushedStates[DSControllerState_Start],
			   flushedStates[DSControllerState_B],
			   flushedStates[DSControllerState_A],
			   flushedStates[DSControllerState_Y],
			   flushedStates[DSControllerState_X],
			   flushedStates[DSControllerState_L],
			   flushedStates[DSControllerState_R],
			   flushedStates[DSControllerState_Debug],
			   flushedStates[DSControllerState_Lid]);
	
	// Setup the DS touch pad.
	CommonSettings.StylusPressure = (int)[self stylusPressure];
	CommonSettings.StylusJitter = ([self stylusEnableJitter]) ? true : false;
	
	if (isTouchDown)
	{
		NDS_setTouchPos((u16)theLocation.x, (u16)theLocation.y);
	}
	else
	{
		NDS_releaseTouch();
	}
	
	// Setup the inputs from SLOT-2 devices.
	const NDS_SLOT2_TYPE slot2DeviceType = slot2_GetSelectedType();
	switch (slot2DeviceType)
	{
		case NDS_SLOT2_GUITARGRIP:
			guitarGrip_setKey(flushedStates[DSControllerState_GuitarGrip_Green],
							  flushedStates[DSControllerState_GuitarGrip_Red],
							  flushedStates[DSControllerState_GuitarGrip_Yellow],
							  flushedStates[DSControllerState_GuitarGrip_Blue]);
			break;
			
		case NDS_SLOT2_EASYPIANO:
			piano_setKey(flushedStates[DSControllerState_Piano_C],
						 flushedStates[DSControllerState_Piano_CSharp],
						 flushedStates[DSControllerState_Piano_D],
						 flushedStates[DSControllerState_Piano_DSharp],
						 flushedStates[DSControllerState_Piano_E],
						 flushedStates[DSControllerState_Piano_F],
						 flushedStates[DSControllerState_Piano_FSharp],
						 flushedStates[DSControllerState_Piano_G],
						 flushedStates[DSControllerState_Piano_GSharp],
						 flushedStates[DSControllerState_Piano_A],
						 flushedStates[DSControllerState_Piano_ASharp],
						 flushedStates[DSControllerState_Piano_B],
						 flushedStates[DSControllerState_Piano_HighC]);
			break;
			
		case NDS_SLOT2_PADDLE:
		{
			const u16 newPaddleValue = Paddle_GetValue() + (u16)[self paddleAdjust];
			Paddle_SetValue(newPaddleValue);
			break;
		}
			
		default:
			break;
	}
	
	// Setup the DS mic.
	AudioGenerator *selectedGenerator = &nullSampleGenerator;
	
	if (isMicPressed)
	{
		switch (theMicMode)
		{
			case MICMODE_INTERNAL_NOISE:
				selectedGenerator = &internalNoiseGenerator;
				break;
				
			case MICMODE_WHITE_NOISE:
				selectedGenerator = &whiteNoiseGenerator;
				break;
				
			case MICMODE_SINE_WAVE:
				selectedGenerator = &sineWaveGenerator;
				break;
				
			case MICMODE_AUDIO_FILE:
				if (selectedAudioFileGenerator != NULL)
				{
					selectedGenerator = selectedAudioFileGenerator;
				}
				break;
				
			default:
				break;
		}
	}
	else
	{
		selectedGenerator = &nullSampleGenerator;
		internalNoiseGenerator.setSamplePosition(0);
		sineWaveGenerator.setCyclePosition(0.0);
		
		if (selectedAudioFileGenerator != NULL)
		{
			selectedAudioFileGenerator->setSamplePosition(0);
		}
	}
	
	_useHardwareMic = (isMicPressed) ? NO : YES;
	softwareMicSampleGenerator = selectedGenerator;
	NDS_setMic(isMicPressed);
}

- (void) flushEmpty
{
	// Setup the DS pad.
	NDS_setPad(false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false,
			   false);
	
	// Setup the DS touch pad.
	NDS_releaseTouch();
	
	// Setup the inputs from SLOT-2 devices.
	const NDS_SLOT2_TYPE slot2DeviceType = slot2_GetSelectedType();
	switch (slot2DeviceType)
	{
		case NDS_SLOT2_GUITARGRIP:
			guitarGrip_setKey(false,
							  false,
							  false,
							  false);
			break;
			
		case NDS_SLOT2_EASYPIANO:
			piano_setKey(false,
						 false,
						 false,
						 false,
						 false,
						 false,
						 false,
						 false,
						 false,
						 false,
						 false,
						 false,
						 false);
			break;
			
		case NDS_SLOT2_PADDLE:
			// Do nothing.
			break;
			
		default:
			break;
	}
	
	// Setup the DS mic.
	_useHardwareMic = NO;
	softwareMicSampleGenerator = &nullSampleGenerator;
	NDS_setMic(false);
}

- (void) reset
{
	for (size_t i = 0; i < DSControllerState_StatesCount; i++)
	{
		memset(ndsInput, 0, sizeof(ndsInput));
	}
	
	[self setAutohold:NO];
	[self setMicLevel:0.0f];
	
	_isAutoholdCleared = YES;
	_availableMicSamples = 0;
	_hwMicLevelList->clear();
}

- (void) clearMicLevelMeasure
{
	_hwMicLevelList->clear();
}

- (void) updateMicLevel
{
	float avgMicLevel = 0.0f;
	size_t recordedLevelCount = _hwMicLevelList->size();
	
	for(size_t i = 0; i < recordedLevelCount; i++)
	{
		avgMicLevel += (*_hwMicLevelList)[i];
	}
	
	if (recordedLevelCount > 0)
	{
		avgMicLevel /= _hwMicLevelList->size();
	}
	else
	{
		avgMicLevel = 0.0f;
	}
	
	NSAutoreleasePool *tempPool = [[NSAutoreleasePool alloc] init];
	[self setMicLevel:avgMicLevel];
	
	if (delegate != nil && [delegate respondsToSelector:@selector(doMicLevelUpdateFromController:)])
	{
		[[self delegate] doMicLevelUpdateFromController:self];
	}
	
	[tempPool release];
}

- (uint8_t) handleMicSampleRead:(CoreAudioInput *)caInput softwareMic:(AudioGenerator *)sampleGenerator
{
	uint8_t theSample = MIC_NULL_SAMPLE_VALUE;
	
	if (_useHardwareMic && (caInput != NULL))
	{
		if (caInput->GetPauseState())
		{
			return theSample;
		}
		else
		{
			if (_availableMicSamples == 0)
			{
				_availableMicSamples = CAInputDevice->Pull();
			}
			
			caInput->_samplesConverted->read(&theSample, 1);
			theSample >>= 1; // Samples from CoreAudio are 8-bit, so we need to convert the sample to 7-bit
			_availableMicSamples--;
		}
	}
	else
	{
		theSample = sampleGenerator->generateSample();
	}
	
	_hwMicLevelList->push_back(fabs((float)theSample - MIC_NULL_SAMPLE_VALUE));
	return theSample;
}

- (void) handleMicHardwareStateChanged:(CoreAudioInputDeviceInfo *)deviceInfo
							 isEnabled:(BOOL)isHardwareEnabled
							  isLocked:(BOOL)isHardwareLocked
{
	NSAutoreleasePool *tempPool = [[NSAutoreleasePool alloc] init];
	
	if (deviceInfo->objectID == kAudioObjectUnknown)
	{
		[self setHardwareMicInfoString:@"No hardware input detected."];
		[self setHardwareMicNameString:@"No hardware input detected."];
		[self setHardwareMicManufacturerString:@"No hardware input detected."];
		[self setHardwareMicSampleRateString:@"No hardware input detected."];
		
	}
	else
	{
		[self setHardwareMicInfoString:[NSString stringWithFormat:@"%@\nSample Rate: %1.1f Hz",
										(NSString *)deviceInfo->name,
										(double)deviceInfo->sampleRate]];
		[self setHardwareMicNameString:(NSString *)deviceInfo->name];
		[self setHardwareMicManufacturerString:(NSString *)deviceInfo->manufacturer];
		[self setHardwareMicSampleRateString:[NSString stringWithFormat:@"%1.1f Hz", (double)deviceInfo->sampleRate]];
	}
	
	[self clearMicLevelMeasure];
	[self setMicLevel:0.0f];
	
	if (delegate != nil && [delegate respondsToSelector:@selector(doMicHardwareStateChangedFromController:isEnabled:isLocked:)])
	{
		[[self delegate] doMicHardwareStateChangedFromController:self
													   isEnabled:isHardwareEnabled
														isLocked:isHardwareLocked];
	}
	
	[tempPool release];
}

- (void) handleMicHardwareGainChanged:(float)gainValue
{
	if (delegate != nil && [delegate respondsToSelector:@selector(doMicHardwareGainChangedFromController:gain:)])
	{
		NSAutoreleasePool *tempPool = [[NSAutoreleasePool alloc] init];
		[[self delegate] doMicHardwareGainChangedFromController:self gain:gainValue];
		[tempPool release];
	}
}

@end

void CAResetCallback(void *inParam1, void *inParam2)
{
	CocoaDSController *cdsController = (CocoaDSController *)inParam1;
	[cdsController CAInputDevice]->Start();
}

uint8_t CASampleReadCallback(void *inParam1, void *inParam2)
{
	CocoaDSController *cdsController = (CocoaDSController *)inParam1;
	return [cdsController handleMicSampleRead:[cdsController CAInputDevice] softwareMic:[cdsController softwareMicSampleGenerator]];
}

void CAHardwareStateChangedCallback(CoreAudioInputDeviceInfo *deviceInfo,
									const bool isHardwareEnabled,
									const bool isHardwareLocked,
									void *inParam1,
									void *inParam2)
{
	CocoaDSController *cdsController = (CocoaDSController *)inParam1;
	[cdsController handleMicHardwareStateChanged:(CoreAudioInputDeviceInfo *)deviceInfo
									   isEnabled:((isHardwareEnabled) ? YES : NO)
										isLocked:((isHardwareLocked) ? YES : NO)];
}

void CAHardwareGainChangedCallback(float normalizedGain, void *inParam1, void *inParam2)
{
	CocoaDSController *cdsController = (CocoaDSController *)inParam1;
	[cdsController handleMicHardwareGainChanged:normalizedGain];
}
