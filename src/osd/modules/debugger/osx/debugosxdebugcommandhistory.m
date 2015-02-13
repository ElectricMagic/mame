// license:BSD-3-Clause
// copyright-holders:Vas Crabb
//============================================================
//
//  debugosxdebugcommandhistory.m - MacOS X Cocoa debug window handling
//
//  Copyright (c) 1996-2015, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

//============================================================
//  MAMEDebugView class
//============================================================

#import "debugosxdebugcommandhistory.h"


@implementation MAMEDebugCommandHistory

+ (NSInteger)defaultLength {
	return 100;
}


- (id)init {
	if (!(self = [super init]))
		return nil;
	length = [[self class] defaultLength];
	position = -1;
	current = nil;
	history = [[NSMutableArray alloc] initWithCapacity:length];
	return self;
}


- (void)dealloc {
	if (current != nil)
		[current release];
	if (history != nil)
		[history release];
	[super dealloc];
}


- (NSInteger)length {
	return length;
}


- (void)setLength:(NSInteger)l {
	length = l;
	while ([history count] > length)
		[history removeLastObject];
}


- (void)add:(NSString *)entry {
	if (([history count] == 0) || ![[history objectAtIndex:0] isEqualToString:entry]) {
		[history insertObject:entry atIndex:0];
		while ([history count] > length)
			[history removeLastObject];
	}
	position = -1;
}


- (NSString *)previous:(NSString *)cur {
	if ((position + 1) < [history count]) {
		if (position < 0) {
			[current autorelease];
			current = [cur copy];
		}
		return [history objectAtIndex:++position];
	} else {
		return nil;
	}
}


- (NSString *)next:(NSString *)cur {
	if (position > 0) {
		return [history objectAtIndex:--position];
	} else if (position == 0) {
		position--;
		return [[current retain] autorelease];
	} else {
		return nil;
	}
}


- (void)reset {
	position = -1;
	if (current != nil) {
		[current release];
		current = nil;
	}
}


- (void)clear {
	position = -1;
	if (current != nil) {
		[current release];
		current = nil;
	}
	[history removeAllObjects];
}

@end
