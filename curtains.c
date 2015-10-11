#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFBase.h>
#include <ApplicationServices/ApplicationServices.h>

typedef int CGSSessionID;
CG_EXTERN CFDictionaryRef CGSCopyCurrentSessionDictionary(void);
CG_EXTERN CGError
CGSSessionCreateSessionIDWithOptions(CFStringRef path,
                                     CFArrayRef argv,
                                     int flags,
                                     CGSSessionID *outSession);
CG_EXTERN CGError CGSReleaseSession(CGSSessionID session);

CGError lockScreen(CFStringRef lockMessage, CGSSessionID *outSession)
{
	CFDictionaryRef dict;
	CFNumberRef number;
	CGSSessionID currentSession, lockSession;
	CFStringRef lockCmd, currentSessionStr;
	size_t lockMsgLen;
	char *lockMsgC;
	CFMutableStringRef lockMsgHex;
	CFMutableArrayRef lockCmdArgs;

	if (lockMessage == NULL)
		lockMessage = CFSTR("");

	lockMsgLen = CFStringGetLength(lockMessage);
	lockMsgC = malloc(sizeof(char) * (lockMsgLen + 1));
	CFStringGetCString(lockMessage,
	                   lockMsgC,
	                   lockMsgLen + 1,
	                   kCFStringEncodingUTF8);
	lockMsgHex = CFStringCreateMutable(NULL, lockMsgLen * 2);
	for (int i = 0; i < lockMsgLen; i++) {
		CFStringAppendFormat(lockMsgHex,
		                     NULL,
		                     CFSTR("%02x"),
		                     lockMsgC[i]);
	}
	free(lockMsgC);

	dict = CGSCopyCurrentSessionDictionary();
	number = (CFNumberRef)
	         CFDictionaryGetValue(dict, CFSTR("kCGSSessionIDKey"));
	CFNumberGetValue(number, kCFNumberIntType, &currentSession);

	lockCmd = CFSTR("/System/Library/CoreServices/RemoteManagement"
	                "/AppleVNCServer.bundle/Contents/Support"
	                "/LockScreen.app/Contents/MacOS/LockScreen");
	lockCmdArgs = CFArrayCreateMutable(NULL, 0, NULL);
	CFArrayAppendValue(lockCmdArgs, lockCmd);
	CFArrayAppendValue(lockCmdArgs, CFSTR("-session"));
	currentSessionStr =
		CFStringCreateWithFormat(NULL,
		                         NULL,
		                         CFSTR("%i"),
		                         currentSession);
	CFArrayAppendValue(lockCmdArgs, currentSessionStr);
	CFArrayAppendValue(lockCmdArgs, CFSTR("-msgHex"));
	CFArrayAppendValue(lockCmdArgs, lockMsgHex);

	CGSSessionCreateSessionIDWithOptions(lockCmd,
	                                     lockCmdArgs,
	                                     3,
	                                     &lockSession);
	
	if (outSession)
		*outSession = lockSession;

	CFRelease(currentSessionStr);
	CFRelease(lockCmdArgs);
	CFRelease(lockCmd);
	CFRelease(dict);
	CFRelease(lockMsgHex);

	return 0;
}

void unlockScreen()
{
	CFNotificationCenterRef dc =
		CFNotificationCenterGetDistributedCenter();
	CFNotificationCenterPostNotificationWithOptions(
		dc,
		CFSTR("com.apple.remotedesktop.stopLockScreen"),
		NULL,
		NULL,
		kCFNotificationPostToAllSessions
	);
}

int main(int argc, char **argv)
{
	if (argc == 2 && !strcmp(argv[1], "off")) {
		unlockScreen();
	} else if (argc == 3 && !strcmp(argv[1], "on")) {
		CFStringRef str =
			CFStringCreateWithCString(NULL,
			                          argv[2],
			                          kCFStringEncodingUTF8);
		lockScreen(str, NULL);
		CFRelease(str);
	} else if (argc == 2 && !strcmp(argv[1], "on")) {
		lockScreen(NULL, NULL);
	} else {
		printf("usage: %s on [MESSAGE]\n", argv[0]);
		printf("       %s off\n", argv[0]);
		return 1;
	}

	return 0;
}
