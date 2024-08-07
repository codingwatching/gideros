
 /*
     File: RosyWriterCapturePipeline.h
 Abstract: The class that creates and manages the AVCaptureSession
  Version: 2.1
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2014 Apple Inc. All Rights Reserved.
 
 */


#import <AVFoundation/AVFoundation.h>

@protocol CameraCapturePipelineDelegate;

@interface CameraCapturePipeline : NSObject

- (void)setDelegate:(id<CameraCapturePipelineDelegate>)delegate callbackQueue:(dispatch_queue_t)delegateCallbackQueue; // delegate is weak referenced

// These methods are synchronous
- (void)startRunning;
- (void)stopRunning;
- (void)setOrientation:(AVCaptureVideoOrientation) angle;


@property(readwrite) BOOL renderingEnabled; // When set to false the GPU will not be used after the setRenderingEnabled: call returns.
@property(readwrite) BOOL frontFacing;
@property(nonatomic, retain) NSString *camdev;

@property(readwrite) AVCaptureVideoOrientation recordingOrientation; // client can set the orientation for the recorded movie

- (CGAffineTransform)transformFromVideoBufferOrientationToOrientation:(AVCaptureVideoOrientation)orientation withAutoMirroring:(BOOL)mirroring; // only valid after startRunning has been called
- (void) getVideoWidth:(int *)width andHeight:(int *)height;
- (void) getStillWidth:(int *)width andHeight:(int *)height;
- (BOOL) takePicture:(void (^)(NSData *image)) handler;
- (BOOL) setFlash:(int) mode;

// Stats
@property(readonly) float videoFrameRate;
@property(readonly) CMVideoDimensions videoDimensions;
@property(nonatomic, readwrite) AVCaptureVideoOrientation videoOrientation;

@end

@protocol CameraCapturePipelineDelegate <NSObject>
@required

- (void)capturePipeline:(CameraCapturePipeline *)capturePipeline didStopRunningWithError:(NSError *)error;

// Preview
- (void)capturePipeline:(CameraCapturePipeline *)capturePipeline previewPixelBufferReadyForDisplay:(CVPixelBufferRef)previewPixelBuffer;
- (void)capturePipelineDidRunOutOfPreviewBuffers:(CameraCapturePipeline *)capturePipeline;

@end
