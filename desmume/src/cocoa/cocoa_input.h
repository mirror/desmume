/*
	Copyright (C) 2011 Roger Manuel
	Copyright (C) 2012-2015 DeSmuME Team

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

#import <Cocoa/Cocoa.h>
#include <libkern/OSAtomic.h>
#include <vector>

@class CocoaDSController;
class CoreAudioInput;
struct CoreAudioInputDeviceInfo;
class AudioGenerator;
class AudioSampleBlockGenerator;

enum
{
	DSControllerState_Right = 0,
	DSControllerState_Left,
	DSControllerState_Down,
	DSControllerState_Up,
	DSControllerState_Select,
	DSControllerState_Start,
	DSControllerState_B,
	DSControllerState_A,
	DSControllerState_Y,
	DSControllerState_X,
	DSControllerState_L,
	DSControllerState_R,
	DSControllerState_Debug,
	DSControllerState_Lid,
	
	DSControllerState_Touch,
	DSControllerState_Microphone,
	
	DSControllerState_GuitarGrip_Green,
	DSControllerState_GuitarGrip_Red,
	DSControllerState_GuitarGrip_Yellow,
	DSControllerState_GuitarGrip_Blue,
	DSControllerState_Piano_C,
	DSControllerState_Piano_CSharp,
	DSControllerState_Piano_D,
	DSControllerState_Piano_DSharp,
	DSControllerState_Piano_E,
	DSControllerState_Piano_F,
	DSControllerState_Piano_FSharp,
	DSControllerState_Piano_G,
	DSControllerState_Piano_GSharp,
	DSControllerState_Piano_A,
	DSControllerState_Piano_ASharp,
	DSControllerState_Piano_B,
	DSControllerState_Piano_HighC,
	DSControllerState_Paddle,
	
	DSControllerState_StatesCount
};

typedef struct
{
	bool state;
	bool turbo;
	bool autohold;
	uint16_t turboPattern;
} NDSInput;

@protocol CocoaDSControllerDelegate <NSObject>

@optional
- (void) doMicLevelUpdateFromController:(CocoaDSController *)cdsController;
- (void) doMicHardwareStateChangedFromController:(CocoaDSController *)cdsController
									   isEnabled:(BOOL)isHardwareEnabled
										isLocked:(BOOL)isHardwareLocked;

- (void) doMicHardwareGainChangedFromController:(CocoaDSController *)cdsController gain:(float)gainValue;

@end

@interface CocoaDSController : NSObject
{
	id <CocoaDSControllerDelegate> delegate;
	
	NDSInput ndsInput[DSControllerState_StatesCount];
	BOOL autohold;
	BOOL _isAutoholdCleared;
	
	NSPoint touchLocation;
	NSInteger paddleAdjust;
	NSInteger stylusPressure;
	BOOL stylusEnableJitter;
	
	float micLevel;
	BOOL hardwareMicMute;
	BOOL _useHardwareMic;
	size_t _availableMicSamples;
	std::vector<uint8_t> *_hwMicLevelList;
	NSInteger micMode;
	
	AudioSampleBlockGenerator *selectedAudioFileGenerator;
	CoreAudioInput *CAInputDevice;
	AudioGenerator *softwareMicSampleGenerator;
	
	NSString *hardwareMicInfoString;
	NSString *hardwareMicNameString;
	NSString *hardwareMicManufacturerString;
	NSString *hardwareMicSampleRateString;
	
	OSSpinLock spinlockControllerState;
}

@property (retain) id <CocoaDSControllerDelegate> delegate;
@property (assign) BOOL autohold;
@property (assign) NSInteger paddleAdjust;
@property (assign) NSInteger stylusPressure;
@property (assign) BOOL stylusEnableJitter;
@property (readonly) BOOL isHardwareMicAvailable;
@property (readonly) BOOL isHardwareMicIdle;
@property (readonly) BOOL isHardwareMicInClip;
@property (assign) float micLevel;
@property (assign) BOOL hardwareMicEnabled;
@property (readonly) BOOL hardwareMicLocked;
@property (assign) float hardwareMicGain;
@property (assign) BOOL hardwareMicMute;
@property (assign) BOOL hardwareMicPause;
@property (assign) BOOL softwareMicState;
@property (assign) NSInteger softwareMicMode;
@property (assign) NSInteger micMode;
@property (readonly) CoreAudioInput *CAInputDevice;
@property (readonly) AudioGenerator *softwareMicSampleGenerator;
@property (assign) AudioSampleBlockGenerator *selectedAudioFileGenerator;
@property (retain) NSString *hardwareMicInfoString;
@property (retain) NSString *hardwareMicNameString;
@property (retain) NSString *hardwareMicManufacturerString;
@property (retain) NSString *hardwareMicSampleRateString;

- (void) setControllerState:(BOOL)theState controlID:(const NSUInteger)controlID;
- (void) setControllerState:(BOOL)theState controlID:(const NSUInteger)controlID turbo:(const BOOL)isTurboEnabled;
- (void) setTouchState:(BOOL)theState location:(const NSPoint)theLocation;
- (void) setSineWaveGeneratorFrequency:(const double)freq;
- (void) clearAutohold;
- (void) flush;
- (void) flushEmpty;
- (void) reset;

- (void) clearMicLevelMeasure;
- (void) updateMicLevel;
- (uint8_t) handleMicSampleRead:(CoreAudioInput *)caInput softwareMic:(AudioGenerator *)sampleGenerator;
- (void) handleMicHardwareStateChanged:(CoreAudioInputDeviceInfo *)deviceInfo
							 isEnabled:(BOOL)isHardwareEnabled
							  isLocked:(BOOL)isHardwareLocked;
- (void) handleMicHardwareGainChanged:(float)gainValue;

@end

#ifdef __cplusplus
extern "C"
{
#endif

void CAResetCallback(void *inParam1, void *inParam2);
uint8_t CASampleReadCallback(void *inParam1, void *inParam2);
void CAHardwareStateChangedCallback(CoreAudioInputDeviceInfo *deviceInfo,
									const bool isHardwareEnabled,
									const bool isHardwareLocked,
									void *inParam1,
									void *inParam2);
void CAHardwareGainChangedCallback(float normalizedGain, void *inParam1, void *inParam2);

#ifdef __cplusplus
}
#endif
