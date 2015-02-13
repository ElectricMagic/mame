// license:BSD-3-Clause
// copyright-holders:Vas Crabb
//============================================================
//
//  debugosxdebugview.h - MacOS X Cocoa debug window handling
//
//  Copyright (c) 1996-2015, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#import "debugosxdebugview.h"

#include "debug/debugcpu.h"


static void debugwin_view_update(debug_view &view, void *osdprivate)
{
	[(MAMEDebugView *)osdprivate update];
}


@implementation MAMEDebugView

- (NSColor *)foregroundForAttribute:(UINT8)attrib {
	const CGFloat alpha = (attrib & DCA_DISABLED) ? 0.5 : 1.0;
	if (attrib & DCA_COMMENT)
		return [NSColor colorWithCalibratedRed:0.0 green:0.375 blue:0.0 alpha:1.0];
	else if (attrib & DCA_INVALID)
		return [NSColor colorWithCalibratedRed:0.0 green:0.0 blue:1.0 alpha:alpha];
	else if (attrib & DCA_CHANGED)
		return [NSColor colorWithCalibratedRed:0.875 green:0.0 blue:0.0 alpha:alpha];
	else
		return [NSColor colorWithCalibratedWhite:0.0 alpha:alpha];
}


- (NSColor *)backgroundForAttribute:(UINT8)attrib {
	if ((attrib & DCA_SELECTED) && (attrib & DCA_CURRENT)) {
		if ([[self window] isKeyWindow] && ([[self window] firstResponder] == self))
			return [NSColor colorWithCalibratedRed:0.875 green:0.625 blue:0.875 alpha:1.0];
		else
			return [NSColor colorWithCalibratedRed:0.875 green:0.5 blue:0.625 alpha:1.0];
	} else if (attrib & DCA_CURRENT) {
		return [NSColor colorWithCalibratedRed:1.0 green:0.625 blue:0.625 alpha:1.0];
	} else if (attrib & DCA_SELECTED) {
		if ([[self window] isKeyWindow] && ([[self window] firstResponder] == self))
			return [NSColor colorWithCalibratedRed:0.75 green:0.875 blue:1.0 alpha:1.0];
		else
			return [NSColor colorWithCalibratedWhite:0.875 alpha:1.0];
	} else if (attrib & DCA_ANCILLARY) {
		return [NSColor colorWithCalibratedWhite:0.75 alpha:1.0];
	} else {
		return [NSColor colorWithCalibratedWhite:1.0 alpha:1.0];
	}
}


- (debug_view_xy)convertLocation:(NSPoint)location {
	debug_view_xy position;
	position.x = lround(floor(location.x / fontWidth));
	position.y = lround(floor(location.y / fontHeight));
	if (position.x < 0)
		position.x = 0;
	else if (position.x >= totalWidth)
		position.x = totalWidth - 1;
	if (position.y < 0)
		position.y = 0;
	else if (position.y >= totalHeight)
		position.y = totalHeight - 1;
	return position;
}


- (void)convertBounds:(NSRect)b toPosition:(debug_view_xy *)origin size:(debug_view_xy *)size {
	origin->x = lround(floor(b.origin.x / fontWidth));
	origin->y = lround(floor(b.origin.y / fontHeight));
	size->x = lround(ceil((b.origin.x + b.size.width) / fontWidth)) - origin->x;
	size->y = lround(ceil((b.origin.y + b.size.height) / fontHeight)) - origin->y;
}


- (void)recomputeVisible {
	if ([self window] != nil) {
		debug_view_xy	origin, size;

		// this gets all the characters that are at least paritally visible
		[self convertBounds:[self visibleRect] toPosition:&origin size:&size];

		// need to render entire lines or we get screwed up characters when widening views
		origin.x = 0;
		size.x = totalWidth;

		// tell them what we think
		view->set_visible_size(size);
		view->set_visible_position(origin);
		originLeft = origin.x;
		originTop = origin.y;
	}
}


- (void)typeCharacterAndScrollToCursor:(char)ch {
	if (view->cursor_supported()) {
		debug_view_xy oldPos = view->cursor_position();
		view->process_char(ch);
		{
			debug_view_xy newPos = view->cursor_position();
			if ((newPos.x != oldPos.x) || (newPos.y != oldPos.y)) {
				[self scrollRectToVisible:NSMakeRect(newPos.x * fontWidth,
													 newPos.y * fontHeight,
													 fontWidth,
													 fontHeight)];
			}
		}
	} else {
		view->process_char(ch);
	}
}


+ (NSFont *)defaultFont {
	// maybe we should get the configured system fixed-width font...
	return [NSFont fontWithName:@"Monaco" size:10];
}


- (id)initWithFrame:(NSRect)f type:(debug_view_type)t machine:(running_machine &)m {
	if (!(self = [super initWithFrame:f]))
		return nil;
	type = t;
	machine = &m;
	view = machine->debug_view().alloc_view((debug_view_type)type, debugwin_view_update, self);
	if (view == nil) {
		[self release];
		return nil;
	}
	totalWidth = totalHeight = 0;
	originLeft = originTop = 0;
	[self setFont:[[self class] defaultFont]];
	return self;
}


- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	if (font != nil)
		[font release];
	[super dealloc];
}


- (void)update {
	debug_view_xy	newSize, newOrigin;

	// resize our frame if the total size has changed
	newSize = view->total_size();
	if ((newSize.x != totalWidth) || (newSize.y != totalHeight)) {
		[self setFrameSize:NSMakeSize(fontWidth * newSize.x, fontHeight * newSize.y)];
		totalWidth = newSize.x;
		totalHeight = newSize.y;
	}

	// scroll the view if we're being told to
	newOrigin = view->visible_position();
	if (newOrigin.y != originTop) {
		[self scrollPoint:NSMakePoint([self visibleRect].origin.x, newOrigin.y * fontHeight)];
		originTop = newOrigin.y;
	}

	// recompute the visible area and mark as dirty
	[self recomputeVisible];
	[self setNeedsDisplay:YES];
}


- (NSSize)maximumFrameSize {
	debug_view_xy max = view->total_size();
	return NSMakeSize(max.x * fontWidth, max.y * fontHeight);
}


- (NSFont *)font {
	return [[font retain] autorelease];
}


- (void)setFont:(NSFont *)f {
	[font autorelease];
	font = [f retain];
	fontWidth = [font maximumAdvancement].width;
	fontHeight = ceil([font ascender] - [font descender]);
	fontAscent = [font ascender];
	[[self enclosingScrollView] setLineScroll:fontHeight];
	totalWidth = totalHeight = 0;
	[self update];
}


- (void)windowDidBecomeKey:(NSNotification *)notification {
	NSWindow *win = [notification object];
	if ((win == [self window]) && ([win firstResponder] == self) && view->cursor_supported())
		[self setNeedsDisplay:YES];
}


- (void)windowDidResignKey:(NSNotification *)notification {
	NSWindow *win = [notification object];
	if ((win == [self window]) && ([win firstResponder] == self) && view->cursor_supported())
		[self setNeedsDisplay:YES];
}


- (BOOL)acceptsFirstResponder {
	return view->cursor_supported();
}


- (BOOL)becomeFirstResponder {
	if (view->cursor_supported()) {
		debug_view_xy pos;
		view->set_cursor_visible(true);
		pos = view->cursor_position();
		[self scrollRectToVisible:NSMakeRect(pos.x * fontWidth, pos.y * fontHeight, fontWidth, fontHeight)];
		[self setNeedsDisplay:YES];
		return [super becomeFirstResponder];
	} else {
		return NO;
	}
}


- (BOOL)resignFirstResponder {
	if (view->cursor_supported())
		[self setNeedsDisplay:YES];
	return [super resignFirstResponder];
}


- (void)viewDidMoveToSuperview {
	[[self enclosingScrollView] setLineScroll:fontHeight];
	[super viewDidMoveToSuperview];
}


- (void)viewDidMoveToWindow {
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidBecomeKeyNotification object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidResignKeyNotification object:nil];
	if ([self window] != nil) {
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(windowDidBecomeKey:)
													 name:NSWindowDidBecomeKeyNotification
												   object:[self window]];
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(windowDidResignKey:)
													 name:NSWindowDidResignKeyNotification
												   object:[self window]];
		[self recomputeVisible];
	}
}


- (BOOL)isFlipped {
	return YES;
}


- (void)drawRect:(NSRect)dirtyRect {
	const debug_view_char	*base;
	debug_view_xy			origin, size;
	debug_view_xy			position, clip;
	NSMutableString			*text;
	NSMutableDictionary		*attributes;
	UINT32					pass, row, col;

	// work out how much we need to draw
	[self recomputeVisible];
	origin = view->visible_position();
	size = view->visible_size();
	[self convertBounds:dirtyRect toPosition:&position size:&clip];

	// this gets the text for the whole visible area
	base = view->viewdata();
	if (!base)
		return;

	text = [[NSMutableString alloc] initWithCapacity:clip.x];
	attributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:font, NSFontAttributeName, nil];
	for (pass = 0; pass < 2; pass++) {
		const debug_view_char *data = base + ((position.y - origin.y) * size.x);
		for (row = position.y; row < position.y + clip.y; row++, data += size.x) {
			int attr = -1;

			if ((row < origin.y) || (row >= origin.y + size.y))
				continue;

			// render entire lines to get character alignment right
			for (col = origin.x; col < origin.x + size.x; col++) {
				if ((attr != data[col - origin.x].attrib) && ([text length] > 0)) {
					if (pass == 0) {
						[[self backgroundForAttribute:attr] set];
						[NSBezierPath fillRect:NSMakeRect((col - [text length]) * fontWidth,
														  row * fontHeight,
														  [text length] * fontWidth,
														  fontHeight)];
					} else {
						[attributes setObject:[self foregroundForAttribute:attr]
									   forKey:NSForegroundColorAttributeName];
						[text drawAtPoint:NSMakePoint((col - [text length]) * fontWidth, row * fontHeight)
						   withAttributes:attributes];
					}
					[text setString:@""];
				}
				attr = data[col - origin.x].attrib;
				[text appendFormat:@"%c", data[col - origin.x].byte];
			}
			if ([text length] > 0) {
				if (pass == 0) {
					[[self backgroundForAttribute:attr] set];
					[NSBezierPath fillRect:NSMakeRect((col - [text length]) * fontWidth,
													  row * fontHeight,
													  [text length] * fontWidth,
													  fontHeight)];
				} else {
					[attributes setObject:[self foregroundForAttribute:attr]
								   forKey:NSForegroundColorAttributeName];
					[text drawAtPoint:NSMakePoint((col - [text length]) * fontWidth, row * fontHeight)
					   withAttributes:attributes];
				}
				[text setString:@""];
			}
		}
	}
	[attributes release];
	[text release];
}


- (void)mouseDown:(NSEvent *)event {
	NSPoint	location = [self convertPoint:[event locationInWindow] fromView:nil];
	if (view->cursor_supported()) {
		view->set_cursor_position([self convertLocation:location]);
		view->set_cursor_visible(true);
		[self setNeedsDisplay:YES];
	}
}


- (void)mouseDragged:(NSEvent *)event {
	NSPoint	location = [self convertPoint:[event locationInWindow] fromView:nil];
	if (view->cursor_supported()) {
		[self autoscroll:event];
		view->set_cursor_position([self convertLocation:location]);
		[self setNeedsDisplay:YES];
	}
}


- (void)rightMouseDown:(NSEvent *)event {
	NSPoint	location = [self convertPoint:[event locationInWindow] fromView:nil];
	if (view->cursor_supported()) {
		view->set_cursor_position([self convertLocation:location]);
		view->set_cursor_visible(true);
		[self setNeedsDisplay:YES];
	}
	[super rightMouseDown:event];
}


- (void)keyDown:(NSEvent *)event {
	NSUInteger	modifiers = [event modifierFlags];
	NSString	*str = [event charactersIgnoringModifiers];

	if ([str length] == 1) {
		if (modifiers & NSNumericPadKeyMask) {
			switch ([str characterAtIndex:0]) {
				case NSUpArrowFunctionKey:
					if (modifiers & NSCommandKeyMask)
						view->process_char(DCH_CTRLHOME);
					else
						view->process_char(DCH_UP);
					return;
				case NSDownArrowFunctionKey:
					if (modifiers & NSCommandKeyMask)
						view->process_char(DCH_CTRLEND);
					else
						view->process_char(DCH_DOWN);
					return;
				case NSLeftArrowFunctionKey:
					if (modifiers & NSCommandKeyMask)
						[self typeCharacterAndScrollToCursor:DCH_HOME];
					else if (modifiers & NSAlternateKeyMask)
						[self typeCharacterAndScrollToCursor:DCH_CTRLLEFT];
					else
						[self typeCharacterAndScrollToCursor:DCH_LEFT];
					return;
				case NSRightArrowFunctionKey:
					if (modifiers & NSCommandKeyMask)
						[self typeCharacterAndScrollToCursor:DCH_END];
					else if (modifiers & NSAlternateKeyMask)
						[self typeCharacterAndScrollToCursor:DCH_CTRLRIGHT];
					else
						[self typeCharacterAndScrollToCursor:DCH_RIGHT];
					return;
				default:
					[self interpretKeyEvents:[NSArray arrayWithObject:event]];
					return;
			}
		} else if (modifiers & NSFunctionKeyMask) {
			switch ([str characterAtIndex:0]) {
				case NSPageUpFunctionKey:
					if (modifiers & NSAlternateKeyMask) {
						view->process_char(DCH_PUP);
						return;
					}
				case NSPageDownFunctionKey:
					if (modifiers & NSAlternateKeyMask) {
						view->process_char(DCH_PDOWN);
						return;
					}
				default:
					;
			}
			[super keyDown:event];
			return;
		}
	}
	[self interpretKeyEvents:[NSArray arrayWithObject:event]];
}


- (void)insertTab:(id)sender {
	if ([[self window] firstResponder] == self)
		[[self window] selectNextKeyView:self];
}


- (void)insertBacktab:(id)sender {
	if ([[self window] firstResponder] == self)
		[[self window] selectPreviousKeyView:self];
}


- (void)insertNewline:(id)sender {
	debug_cpu_get_visible_cpu(*machine)->debug()->single_step();
}


- (void)insertText:(id)string {
	NSUInteger	len;
	NSRange		found;
	if ([string isKindOfClass:[NSAttributedString class]])
		string = [string string];
	for (len = [string length], found = NSMakeRange(0, 0);
		 found.location < len;
		 found.location += found.length) {
		found = [string rangeOfComposedCharacterSequenceAtIndex:found.location];
		if (found.length == 1) {
			unichar ch = [string characterAtIndex:found.location];
			if ((ch >= 32) && (ch < 127))
				[self typeCharacterAndScrollToCursor:ch];
		}
	}
}

@end
