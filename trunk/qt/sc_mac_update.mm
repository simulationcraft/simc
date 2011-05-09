#include "sc_autoupdate.h"

#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <Sparkle/Sparkle.h>

class CocoaInitializer::Private 
{
	public:
		NSAutoreleasePool* autoReleasePool_;
};

CocoaInitializer::CocoaInitializer()
{
	d = new CocoaInitializer::Private();
	NSApplicationLoad();
	d->autoReleasePool_ = [[NSAutoreleasePool alloc] init];
}

CocoaInitializer::~CocoaInitializer()
{
	[d->autoReleasePool_ release];
	delete d;
}

class SparkleAutoUpdater::Private
{
	public:
		SUUpdater* updater;
};

SparkleAutoUpdater::SparkleAutoUpdater(const QString& aUrl)
{
	d = new Private;

	d->updater = [SUUpdater sharedUpdater];
	[d->updater retain];

	NSURL* url = [NSURL URLWithString:
			[NSString stringWithUTF8String: aUrl.toUtf8().data()]];
	[d->updater setFeedURL: url];
}

SparkleAutoUpdater::~SparkleAutoUpdater()
{
	[d->updater release];
	delete d;
}

void SparkleAutoUpdater::checkForUpdates()
{
	[d->updater checkForUpdatesInBackground];
}
